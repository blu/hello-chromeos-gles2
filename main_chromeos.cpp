#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <wayland-client.h>
#include <drm_fourcc.h>
#include "linux-dmabuf-unstable-v1-client-protocol.h"

#include <string>
#include <sstream>
#include <iomanip>

#include "stream.hpp"
#include "util_file.hpp"
#include "util_misc.hpp"
#include "scoped.hpp"
#include "pure_macro.hpp"

#include "egl_ext.h"
#include "gles_ext.h"
#include "timer.h"

// verify iostream-free status
#if _GLIBCXX_IOSTREAM
#error rogue iostream acquired
#endif

namespace stream { // enclosed declarations initialized by main()
in cin;   // std::cin substitute
out cout; // std::cout substitute
out cerr; // std::cerr substitute
} // namespace stream

namespace util {

const char arg_prefix[] = "-";
const char arg_app[]    = "app";

template < typename T >
class generic_free {
public:
	void operator()(T* arg)
	{
		free(arg);
	}
};

template <>
class scoped_functor< FILE > {
public:
	void operator()(FILE* arg)
	{
		fclose(arg);
	}
};

} // namespace util

namespace { // anonymous

const char arg_nframes[]       = "frames";
const char arg_screen[]        = "screen";
const char arg_bitness[]       = "bitness";
const char arg_fsaa[]          = "fsaa";
const char arg_drawcalls[]     = "drawcalls";

#define FULLSCREEN_WIDTH 1366
#define FULLSCREEN_HEIGHT 768

////////////////////////////////////////////////////////////////////////////////////////////////////
// wayland interfaces
////////////////////////////////////////////////////////////////////////////////////////////////////

wl_compositor *compositor;
zwp_linux_dmabuf_v1 *dmabuf;
wl_shell *shell;

////////////////////////////////////////////////////////////////////////////////////////////////////
// EGL helper
////////////////////////////////////////////////////////////////////////////////////////////////////

struct DRMBuffer {
	// acquired properties of DRM buffer object
	uint32_t handle;   // GEM handle
	uint32_t pitch;    // row pitch in bytes
	uint64_t size;     // total size in bytes
	int32_t dmabuf_fd; // prime file descriptor

	DRMBuffer()
	: handle(0)
	, pitch(0)
	, size(0)
	, dmabuf_fd(-1) {
	}
};

template < typename T, size_t N >
int8_t (& noneval_countof(const T (&)[N]))[N];

#define countof(x) sizeof(noneval_countof(x))

template < typename T, size_t N >
void set_array(T (&arr)[N], const T val) {
	for (size_t i = 0; i < N; ++i)
		arr[i] = val;
}

template < bool >
struct compiler_assert;

template <>
struct compiler_assert< true > {};

struct EGL {
	// general enablement
	EGLDisplay display;     // EGL display
	EGLContext context;     // EGL context
	int32_t drm_fd;         // DRM device file descriptor
	uint32_t fourcc;        // DRM fourcc of the framebuffer

	// primary framebuffer (double buffering)
	DRMBuffer drmbuf[2];    // DRM buffer object, per buffer
	EGLImage image[2];      // EGL image, per buffer
	GLuint rbo[4];          // framebuffer attachment names, per buffer
	GLuint fbo[2];          // framebuffer name, per buffer
	EGLSyncKHR fence[2];    // fence sync, per buffer

	wl_buffer *buffer[2];   // wayland buffer, per buffer (foreign ownership)
	wl_surface *surface;    // wayland surface (foreign ownership)

	int32_t frame_error;
	uint32_t frame_count;

	uint32_t window_w;      // framebuffer width
	uint32_t window_h;      // framebuffer height

	EGL(uint32_t w,
	    uint32_t h)
	: display(EGL_NO_DISPLAY)
	, context(EGL_NO_CONTEXT)
	, drm_fd(-1)
	, surface(nullptr)
	, frame_error(0)
	, frame_count(0)
	, window_w(w)
	, window_h(h)
	{
		const compiler_assert< countof(image) == countof(drmbuf) >  assert_countof_drm __attribute__ ((unused));
		const compiler_assert< countof(image) == countof(rbo) / 2 > assert_countof_rbo __attribute__ ((unused));
		const compiler_assert< countof(image) == countof(fbo) >     assert_countof_fbo __attribute__ ((unused));
		const compiler_assert< countof(image) == countof(fence) >   assert_countof_fence __attribute__ ((unused));
		const compiler_assert< countof(image) == countof(buffer) >  assert_countof_wl_buffer __attribute__ ((unused));

		set_array(image, EGL_NO_IMAGE_KHR);
		set_array(rbo, GLuint(0));
		set_array(fbo, GLuint(0));
		set_array(fence, EGL_NO_SYNC_KHR);
		set_array(buffer, (wl_buffer *) nullptr);

		init_egl_ext();
		init_gles_ext();
	}

	bool initGLES2(
		const unsigned fsaa,
		const unsigned (& bitness)[4]);

	void deinit();

	~EGL()
	{
		deinit();
	}

	int create_fence_sync(const size_t index)
	{
		const EGLSyncKHR new_fence = eglCreateSyncKHR(display, EGL_SYNC_FENCE_KHR, NULL);

		if (EGL_NO_SYNC_KHR == new_fence) {
			return -1;
		}

		fence[index] = new_fence;
		return 0;
	}

	int wait_fence_sync(const size_t index)
	{
		const EGLSyncKHR old_fence = fence[index];

		if (EGL_CONDITION_SATISFIED_KHR != eglClientWaitSyncKHR(
			display,
			old_fence,
			EGL_SYNC_FLUSH_COMMANDS_BIT_KHR,
			EGL_FOREVER_KHR))
		{
			return -1;
		}

		// a fence sync is triggered only once and then abandoned; destroy the old
		// fence sync so it doesn't leak
		eglDestroySyncKHR(display, old_fence);
		fence[index] = EGL_NO_SYNC_KHR;
		return 0;
	}

	void activate_framebuffer(const size_t index) const
	{
		glBindFramebuffer(GL_FRAMEBUFFER, fbo[index]);
		glViewport(0, 0, window_w, window_h);
	}
};

std::string
string_from_EGL_error(
	const EGLint error)
{
	switch (error) {

	case EGL_SUCCESS:
		return "EGL_SUCCESS";

	case EGL_NOT_INITIALIZED:
		return "EGL_NOT_INITIALIZED";

	case EGL_BAD_ACCESS:
		return "EGL_BAD_ACCESS";

	case EGL_BAD_ALLOC:
		return "EGL_BAD_ALLOC";

	case EGL_BAD_ATTRIBUTE:
		return "EGL_BAD_ATTRIBUTE";

	case EGL_BAD_CONTEXT:
		return "EGL_BAD_CONTEXT";

	case EGL_BAD_CONFIG:
		return "EGL_BAD_CONFIG";

	case EGL_BAD_CURRENT_SURFACE:
		return "EGL_BAD_CURRENT_SURFACE";

	case EGL_BAD_DISPLAY:
		return "EGL_BAD_DISPLAY";

	case EGL_BAD_SURFACE:
		return "EGL_BAD_SURFACE";

	case EGL_BAD_MATCH:
		return "EGL_BAD_MATCH";

	case EGL_BAD_PARAMETER:
		return "EGL_BAD_PARAMETER";

	case EGL_BAD_NATIVE_PIXMAP:
		return "EGL_BAD_NATIVE_PIXMAP";

	case EGL_BAD_NATIVE_WINDOW:
		return "EGL_BAD_NATIVE_WINDOW";

	case EGL_CONTEXT_LOST:
		return "EGL_CONTEXT_LOST";
	}

	std::ostringstream s;
	s << "unknown EGL error (0x" << std::hex << std::setw(8) << std::setfill('0') << error << ")";

	return s.str();
}

std::string
string_from_EGL_attrib(
	const EGLint attr)
{
	switch (attr) {

	case EGL_BUFFER_SIZE:
		return "EGL_BUFFER_SIZE";

	case EGL_ALPHA_SIZE:
		return "EGL_ALPHA_SIZE";

	case EGL_BLUE_SIZE:
		return "EGL_BLUE_SIZE";

	case EGL_GREEN_SIZE:
		return "EGL_GREEN_SIZE";

	case EGL_RED_SIZE:
		return "EGL_RED_SIZE";

	case EGL_DEPTH_SIZE:
		return "EGL_DEPTH_SIZE";

	case EGL_STENCIL_SIZE:
		return "EGL_STENCIL_SIZE";

	case EGL_CONFIG_CAVEAT:
		return "EGL_CONFIG_CAVEAT";

	case EGL_CONFIG_ID:
		return "EGL_CONFIG_ID";

	case EGL_LEVEL:
		return "EGL_LEVEL";

	case EGL_MAX_PBUFFER_HEIGHT:
		return "EGL_MAX_PBUFFER_HEIGHT";

	case EGL_MAX_PBUFFER_PIXELS:
		return "EGL_MAX_PBUFFER_PIXELS";

	case EGL_MAX_PBUFFER_WIDTH:
		return "EGL_MAX_PBUFFER_WIDTH";

	case EGL_NATIVE_RENDERABLE:
		return "EGL_NATIVE_RENDERABLE";

	case EGL_NATIVE_VISUAL_ID:
		return "EGL_NATIVE_VISUAL_ID";

	case EGL_NATIVE_VISUAL_TYPE:
		return "EGL_NATIVE_VISUAL_TYPE";

	case EGL_SAMPLES:
		return "EGL_SAMPLES";

	case EGL_SAMPLE_BUFFERS:
		return "EGL_SAMPLE_BUFFERS";

	case EGL_SURFACE_TYPE:
		return "EGL_SURFACE_TYPE";

	case EGL_TRANSPARENT_TYPE:
		return "EGL_TRANSPARENT_TYPE";

	case EGL_TRANSPARENT_BLUE_VALUE:
		return "EGL_TRANSPARENT_BLUE_VALUE";

	case EGL_TRANSPARENT_GREEN_VALUE:
		return "EGL_TRANSPARENT_GREEN_VALUE";

	case EGL_TRANSPARENT_RED_VALUE:
		return "EGL_TRANSPARENT_RED_VALUE";

	case EGL_BIND_TO_TEXTURE_RGB:
		return "EGL_BIND_TO_TEXTURE_RGB";

	case EGL_BIND_TO_TEXTURE_RGBA:
		return "EGL_BIND_TO_TEXTURE_RGBA";

	case EGL_MIN_SWAP_INTERVAL:
		return "EGL_MIN_SWAP_INTERVAL";

	case EGL_MAX_SWAP_INTERVAL:
		return "EGL_MAX_SWAP_INTERVAL";

	case EGL_LUMINANCE_SIZE:
		return "EGL_LUMINANCE_SIZE";

	case EGL_ALPHA_MASK_SIZE:
		return "EGL_ALPHA_MASK_SIZE";

	case EGL_COLOR_BUFFER_TYPE:
		return "EGL_COLOR_BUFFER_TYPE";

	case EGL_RENDERABLE_TYPE:
		return "EGL_RENDERABLE_TYPE";

	case EGL_CONFORMANT:
		return "EGL_CONFORMANT";
	}

	std::ostringstream s;
	s << "unknown EGL attribute (0x" << std::hex << std::setw(8) << std::setfill('0') << attr << ")";

	return s.str();
}

std::string
string_from_GL_error(
	const GLenum error)
{
	switch (error) {

	case GL_NO_ERROR:
		return "GL_NO_ERROR";

	case GL_INVALID_ENUM:
		return "GL_INVALID_ENUM";

	case GL_INVALID_FRAMEBUFFER_OPERATION:
		return "GL_INVALID_FRAMEBUFFER_OPERATION";

	case GL_INVALID_VALUE:
		return "GL_INVALID_VALUE";

	case GL_INVALID_OPERATION:
		return "GL_INVALID_OPERATION";

	case GL_OUT_OF_MEMORY:
		return "GL_OUT_OF_MEMORY";
	}

	std::ostringstream s;
	s << "unknown GL error (0x" << std::hex << std::setw(8) << std::setfill('0') << error << ")";

	return s.str();
}

void registry_global(void *data, wl_registry *registry, uint32_t name, const char *interface, uint32_t version);
void registry_global_remove(void *, wl_registry *, uint32_t);

void redraw(void *, wl_callback *, uint32_t);

const wl_registry_listener registry_listener = {
	.global = registry_global,
	.global_remove = registry_global_remove
};

wl_display *setup_wayland(void)
{
	wl_display *display = wl_display_connect(NULL);

	if (display == NULL)
		return NULL;

	wl_registry *registry = wl_display_get_registry(display);
	wl_registry_add_listener(registry, &registry_listener, NULL);
	wl_display_roundtrip(display);
	wl_registry_destroy(registry);

	return display;
}

void cleanup_wayland(wl_display *display)
{
	wl_shell_destroy(shell);
	zwp_linux_dmabuf_v1_destroy(dmabuf);
	wl_compositor_destroy(compositor);
	wl_display_disconnect(display);
}

void
dmabuf_format(void *data, struct zwp_linux_dmabuf_v1 *zwp_linux_dmabuf, uint32_t format)
{
#if DEBUG
	fprintf(stderr, "%s, 0x%08x, %c%c%c%c\n", __FUNCTION__, format,
		char(format >>  0),
		char(format >>  8),
		char(format >> 16),
		char(format >> 24));

#endif
}

void
dmabuf_modifier(void *data, struct zwp_linux_dmabuf_v1 *zwp_linux_dmabuf,
	uint32_t format, uint32_t modifier_hi, uint32_t modifier_lo)
{
}

const struct zwp_linux_dmabuf_v1_listener dmabuf_listener = {
    .format = dmabuf_format, // up to protocol version 2 zwp_linux_dmabuf_v1_listener::format is called back
    .modifier = dmabuf_modifier // from protocol version 3 onwards zwp_linux_dmabuf_v1_listener::modifier is called back
};

void registry_global(void *data, wl_registry *registry, uint32_t name, const char *interface, uint32_t version)
{
#if DEBUG
    fprintf(stderr, "%s, %s id %d version %d\n", __FUNCTION__, interface, name, version);

#endif
	if (strcmp(interface, wl_compositor_interface.name) == 0) {
		compositor = static_cast< wl_compositor* >(wl_registry_bind(registry, name, &wl_compositor_interface, 3));
	}
	else if (strcmp(interface, zwp_linux_dmabuf_v1_interface.name) == 0) {
		dmabuf = static_cast< zwp_linux_dmabuf_v1* >(wl_registry_bind(registry, name, &zwp_linux_dmabuf_v1_interface, 2));
		zwp_linux_dmabuf_v1_add_listener(dmabuf, &dmabuf_listener, data);
	}
	else if (strcmp(interface, wl_shell_interface.name) == 0) {
		shell = static_cast< wl_shell* >(wl_registry_bind(registry, name, &wl_shell_interface, 1));
	}
}

void registry_global_remove(void *, wl_registry *, uint32_t)
{
}

void
buffer_release(void *data, struct wl_buffer *buffer)
{
#if DEBUG
	fprintf(stderr, "%s, %p\n", __FUNCTION__, data);

#endif
}

const struct wl_buffer_listener buffer_listener = {
	.release = buffer_release
};

void
create_succeeded(void *data,
	struct zwp_linux_buffer_params_v1 *params,
	struct wl_buffer *new_buffer)
{
	wl_buffer_add_listener(new_buffer, &buffer_listener, data);
	zwp_linux_buffer_params_v1_destroy(params);
}

void
create_failed(void *data, struct zwp_linux_buffer_params_v1 *params)
{
	zwp_linux_buffer_params_v1_destroy(params);
}

const struct zwp_linux_buffer_params_v1_listener params_listener = {
	.created = create_succeeded,
	.failed = create_failed
};

wl_buffer *create_buffer_dmabuf(
	int32_t dmabuf_fd,
	uint32_t dmabuf_stride,
	uint32_t dmabuf_format,
	uint32_t width,
	uint32_t height)
{
	uint32_t flags = ZWP_LINUX_BUFFER_PARAMS_V1_FLAGS_Y_INVERT;
	struct zwp_linux_buffer_params_v1 *params;
	struct wl_buffer *buffer;

	params = zwp_linux_dmabuf_v1_create_params(dmabuf);
	zwp_linux_buffer_params_v1_add(
		params,
		dmabuf_fd,
		0, // plane_idx
		0, // offset
		dmabuf_stride,
		0, // modifier_hi
		0); // modifier_lo

	zwp_linux_buffer_params_v1_add_listener(params, &params_listener, NULL);

	buffer = zwp_linux_buffer_params_v1_create_immed(
		params,
		width,
		height,
		dmabuf_format,
		flags);

	wl_buffer_add_listener(buffer, &buffer_listener, NULL);

	return buffer;
}

void shell_surface_ping(void *data, wl_shell_surface *shell_surface, uint32_t serial)
{
	wl_shell_surface_pong(shell_surface, serial);
}

void shell_surface_configure(void *data, wl_shell_surface *shell_surface, uint32_t edges, int32_t width, int32_t height)
{
	// on resize-to-fullscreen we get called back twice: once at the start of transition, and once at the end of transition;
	// both times we get passed the final-target width and height
	// on resize-to-windowed we get called back once;

#if 0 // TODO
	EGL *egl = static_cast< EGL* >(wl_shell_surface_get_user_data(shell_surface));
	egl->window_w = width;
	egl->window_h = height;

#endif
}

const wl_shell_surface_listener shell_surface_listener = {
	.ping = shell_surface_ping,
	.configure = shell_surface_configure,
};

const wl_callback_listener frame_listener = {
	.done = redraw
};

void set_frame_listener(EGL *egl)
{
	wl_callback *callback = wl_surface_frame(egl->surface);
	wl_callback_add_listener(callback, &frame_listener, egl);
}

void redraw(void *data, wl_callback *callback, uint32_t time)
{
	EGL *egl = static_cast< EGL* >(data);

	int32_t frame_error = egl->frame_error;
	uint32_t frame_count = egl->frame_count;

#if QUIRK0001_SYSTEM_CRASH_AT_EXIT == 0
	// don't draw if an error has occurred
	if (frame_error)
		return;

#endif
	const size_t curr_buffer_idx = frame_count & 1;
	const size_t next_buffer_idx = ++frame_count & 1;
	egl->frame_count = frame_count;

	wl_buffer *buffer = egl->buffer[next_buffer_idx];
	wl_surface *surface = egl->surface;
	const uint32_t window_w = egl->window_w;
	const uint32_t window_h = egl->window_h;

#if QUIRK0001_SYSTEM_CRASH_AT_EXIT
	// skip rendering if an error has occurred, but keep flipping the buffers indefinitely
	if (!frame_error) {

#endif
	// emit app's next frame into the buffer that was just presented
	egl->activate_framebuffer(curr_buffer_idx);

	if (!hook::render_frame(egl->fbo[curr_buffer_idx])) {
		egl->frame_error = -1;
		return;
	}

#if QUIRK0001_SYSTEM_CRASH_AT_EXIT
	}

#endif
#if 0
	// create a fence sync for next frame
	if (egl->create_fence_sync(curr_buffer_idx)) {
		egl->frame_error = -1;
		return;
	}

	// block on a previous fence sync to make sure the gpu is done with the other buffer
	if (egl->wait_fence_sync(next_buffer_idx)) {
		egl->frame_error = -1;
		return;
	}

#endif
	// a callback is triggered only once and then abandoned; destroy the old
	// callback that invoked us so it doesn't leak, then create a new callback
	wl_callback_destroy(callback);
	set_frame_listener(egl);

	// update wayland surface from the other buffer and trigger a screen update
	wl_surface_attach(surface, buffer, 0, 0);
	wl_surface_damage(surface, 0, 0, window_w, window_h);
	wl_surface_commit(surface);
}

wl_shell_surface *create_surface(EGL *egl)
{
	wl_surface *surface;
	wl_shell_surface *shell_surface;

	surface = wl_compositor_create_surface(compositor);

	if (surface == NULL)
		return NULL;

	shell_surface = wl_shell_get_shell_surface(shell, surface);

	if (shell_surface == NULL) {
		wl_surface_destroy(surface);
		return NULL;
	}

	egl->surface = surface;
	wl_shell_surface_add_listener(shell_surface, &shell_surface_listener, NULL);
	wl_shell_surface_set_toplevel(shell_surface);
	wl_shell_surface_set_user_data(shell_surface, egl);
	wl_surface_set_user_data(surface, NULL);

	return shell_surface;
}

} // namespace anonymous

namespace util {

bool
reportEGLError(
	FILE* const file)
{
	const EGLint error = eglGetError();

	if (EGL_SUCCESS == error)
		return false;

	fprintf(file, "EGL error: %s\n", string_from_EGL_error(error).c_str());
	return true;
}

bool
reportGLError(
	FILE* const file)
{
	const GLenum error = glGetError();

	if (GL_NO_ERROR == error)
		return false;

	fprintf(file, "GL error: %s\n", string_from_GL_error(error).c_str());
	return true;
}

template <>
class scoped_functor< EGL > {
public:
	void operator()(EGL* arg)
	{
		reportEGLError();
		arg->deinit();
	}
};

} // namespace util

static int create_drm_dumb_bo(
	const int32_t drm_fd,
	const uint32_t width,
	const uint32_t height,
	const uint32_t bpp,
	uint32_t *const handle,
	uint32_t *const pitch,
	uint64_t *const size)
{
	assert(handle);
	assert(pitch);
	assert(size);

	struct drm_mode_create_dumb create_request = {
		.width  = width,
		.height = height,
		.bpp    = bpp
	};
	const int ret = ioctl(drm_fd, DRM_IOCTL_MODE_CREATE_DUMB, &create_request);

	if (ret)
		return ret;

	*handle = create_request.handle;
	*pitch = create_request.pitch;
	*size = create_request.size;
	return 0;
}

static int destroy_drm_dumb_bo(
	const int32_t drm_fd,
	const uint32_t handle)
{
	struct drm_mode_destroy_dumb destroy_request = {
		.handle = handle
	};

	const int ret = ioctl(drm_fd, DRM_IOCTL_MODE_DESTROY_DUMB, &destroy_request);
	return ret;
}

static int create_drm_dmabuf(
	const int32_t drm_fd,
	const uint32_t handle,
	int32_t *const dmabuf_fd)
{
	struct drm_prime_handle prime_request = {
		.handle = handle,
		.flags  = DRM_CLOEXEC | DRM_RDWR,
		.fd     = -1
	};

	const int ret = ioctl(drm_fd, DRM_IOCTL_PRIME_HANDLE_TO_FD, &prime_request);

	if (ret)
		return ret;

	*dmabuf_fd = prime_request.fd;
	return 0;
}

bool EGL::initGLES2(
	const unsigned fsaa,
	const unsigned (& bitness)[4])
{
	using util::scoped_ptr;
	using util::scoped_functor;

	deinit();

	const uint32_t nbits_r = bitness[0];
	const uint32_t nbits_g = bitness[1];
	const uint32_t nbits_b = bitness[2];
	const uint32_t nbits_a = bitness[3];

	fourcc = 0;
	uint32_t bpp = 0;
	switch (nbits_r << 24 | nbits_g << 16 | nbits_b << 8 | nbits_a) {
	case 0x08080808:
		fourcc = DRM_FORMAT_ABGR8888; // DRM_FORMAT_ARGB8888;
		bpp = 32;
		break;
	case 0x08080800:
		fourcc = DRM_FORMAT_XBGR8888; // DRM_FORMAT_XRGB8888;
		bpp = 32;
		break;
	case 0x05060500:
		fourcc = DRM_FORMAT_RGB565;
		bpp = 16;
		break;
	}

	if (0 == fourcc) {
		stream::cerr << "unsupported pixel format requested; abort\n";
		return false;
	}

	scoped_ptr< EGL, scoped_functor > deinit_self(this);

	drm_fd = open("/dev/dri/vgem", O_RDWR | O_CLOEXEC);

	if (drm_fd < 0) {
		stream::cerr << "could not open /dev/dri/vgem: " << strerror(errno) << '\n';
		return false;
	}

	for (size_t i = 0; i < countof(drmbuf); ++i) {
		const int ret = create_drm_dumb_bo(
			drm_fd, window_w, window_h, bpp,
			&drmbuf[i].handle,
			&drmbuf[i].pitch,
			&drmbuf[i].size);

		if (ret) {
			stream::cerr << "could not allocate dumb buffer: " << strerror(ret) << '\n';
			return false;
		}
	}

	for (size_t i = 0; i < countof(drmbuf); ++i) {
		const int ret = create_drm_dmabuf(drm_fd, drmbuf[i].handle, &drmbuf[i].dmabuf_fd);

		if (ret) {
			stream::cerr << "could not export buffer: " << strerror(ret) << '\n';
			return false;
		}
	}

	display = eglGetDisplay(EGL_DEFAULT_DISPLAY);

 	EGLint major;
 	EGLint minor;

	if (EGL_FALSE == eglInitialize(display, &major, &minor)) {
		stream::cerr << "eglInitialize() failed\n";
		return false;
	}

	stream::cout << "eglInitialize() succeeded; major: " << major << ", minor: " << minor << '\n';

	const char* str_version	= eglQueryString(display, EGL_VERSION);
	const char* str_vendor	= eglQueryString(display, EGL_VENDOR);
	const char* str_exten	= eglQueryString(display, EGL_EXTENSIONS);

	stream::cout << "egl version, vendor, extensions:"
		"\n\t" << str_version <<
		"\n\t" << str_vendor <<
		"\n\t" << str_exten << '\n';

	const char* str_exten_nodisp = eglQueryString(EGL_NO_DISPLAY, EGL_EXTENSIONS);

	if (str_exten_nodisp)
		stream::cout << "\negl extensions (no display):\n\t" << str_exten_nodisp << '\n';

	if (NULL == strstr(str_exten, "EGL_KHR_image_base")) {
		stream::cerr << "egl does not support EGL_KHR_image_base\n";
		return false;
	}
	if (NULL == strstr(str_exten, "EGL_KHR_no_config_context")) {
		stream::cerr << "egl does not suppor EGL_KHR_no_config_context\n";
		return false;
	}
	if (NULL == strstr(str_exten, "EGL_KHR_surfaceless_context")) {
		stream::cerr << "egl does not support EGL_KHR_surfaceless_context\n";
		return false;
	}
	if (NULL == strstr(str_exten, "EGL_EXT_image_dma_buf_import")) {
		stream::cerr << "egl does not support EGL_EXT_image_dma_buf_import\n";
		return false;
	}
	if (NULL == strstr(str_exten, "EGL_KHR_fence_sync")) {
		stream::cerr << "egl does not support EGL_KHR_fence_sync\n";
		return false;
	}

	eglBindAPI(EGL_OPENGL_ES_API);

	const EGLint context_attr[] = {
		EGL_CONTEXT_CLIENT_VERSION, 2,
		EGL_NONE
	};

	context = eglCreateContext(
		display,
		EGL_NO_CONFIG_KHR,
		EGL_NO_CONTEXT,
		context_attr);

	if (EGL_NO_CONTEXT == context) {
		stream::cerr << "eglCreateContext() failed\n";
		return false;
	}

	EGLint image_attr[] = {
		EGL_WIDTH,
		EGLint(window_w),
		EGL_HEIGHT,
		EGLint(window_h),
		EGL_LINUX_DRM_FOURCC_EXT,
		EGLint(fourcc),
		EGL_DMA_BUF_PLANE0_FD_EXT,
		-1, // [7]
		EGL_DMA_BUF_PLANE0_OFFSET_EXT,
		EGLint(0),
		EGL_DMA_BUF_PLANE0_PITCH_EXT,
		-1, // [11]
		EGL_NONE
	};

	for (size_t i = 0; i < countof(image); ++i) {
		image_attr[ 7] = EGLint(drmbuf[i].dmabuf_fd);
		image_attr[11] = EGLint(drmbuf[i].pitch);

		image[i] = eglCreateImageKHR(
			display,
			EGL_NO_CONTEXT,
			EGL_LINUX_DMA_BUF_EXT,
			EGLClientBuffer(NULL),
			image_attr);

		if (EGL_NO_IMAGE_KHR == image[i]) {
			stream::cerr << "eglCreateImage() failed\n";
			return false;
		}
	}

	if (EGL_FALSE == eglMakeCurrent(
			display,
			EGL_NO_SURFACE,
			EGL_NO_SURFACE,
			context)) {

		stream::cerr << "eglMakeCurrent() failed\n";
		return false;
	}

	// set the newly-acquired EGLimage as storage to our primary renderbuffer
	glGenRenderbuffers(countof(rbo), rbo);
	glGenFramebuffers(countof(fbo), fbo);

	for (size_t i = 0; i < countof(image); ++i) {
		glBindRenderbuffer(GL_RENDERBUFFER, rbo[i]);
		glEGLImageTargetRenderbufferStorageOES(GL_RENDERBUFFER, image[i]);

		DEBUG_GL_ERR()
	}

	const bool requires_depth = hook::requires_depth();

	if (requires_depth) {
		for (size_t i = 0; i < countof(image); ++i) {
			glBindRenderbuffer(GL_RENDERBUFFER, rbo[countof(image) + i]);
			glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8_OES, window_w, window_h);

			DEBUG_GL_ERR()
		}
	}

	glBindRenderbuffer(GL_RENDERBUFFER, 0);

	for (size_t i = 0; i < countof(image); ++i) {
		glBindFramebuffer(GL_FRAMEBUFFER, fbo[i]);
		glFramebufferRenderbuffer(
			GL_FRAMEBUFFER,
			GL_COLOR_ATTACHMENT0,
			GL_RENDERBUFFER,
			rbo[i]);

		if (requires_depth) {
			glFramebufferRenderbuffer(
				GL_FRAMEBUFFER,
				GL_DEPTH_ATTACHMENT,
				GL_RENDERBUFFER,
				rbo[countof(image) + i]);
			glFramebufferRenderbuffer(
				GL_FRAMEBUFFER,
				GL_STENCIL_ATTACHMENT,
				GL_RENDERBUFFER,
				rbo[countof(image) + i]);
		}

		const GLenum fbo_success = glCheckFramebufferStatus(GL_FRAMEBUFFER);

		if (GL_FRAMEBUFFER_COMPLETE != fbo_success) {
			stream::cerr << "glCheckFramebufferStatus() failed\n";
			return false;
		}
	}

	deinit_self.reset();
	return true;
}

void EGL::deinit()
{
	if (0 != fbo[0]) {
		glDeleteFramebuffers(countof(fbo), fbo);
		set_array(fbo, GLuint(0));
	}

	if (0 != rbo[0]) {
		glDeleteRenderbuffers(countof(rbo), rbo);
		set_array(rbo, GLuint(0));
	}

	for (size_t i = 0; i < countof(fence); ++i) {
		if (EGL_NO_SYNC_KHR == fence[i])
			continue;
		eglDestroySyncKHR(display, fence[i]);
		fence[i] = EGL_NO_SYNC_KHR;
	}

	for (size_t i = 0; i < countof(image); ++i) {
		if (EGL_NO_IMAGE_KHR == image[i])
			continue;
		eglDestroyImageKHR(display, image[i]);
		image[i] = EGL_NO_IMAGE_KHR;
	}

	for (size_t i = 0; i < countof(drmbuf); ++i) {
		if (-1 == drmbuf[i].dmabuf_fd)
			continue;
		close(drmbuf[i].dmabuf_fd);
		drmbuf[i].dmabuf_fd = -1;
	}

	for (size_t i = 0; i < countof(drmbuf); ++i) {
		if (0 == drmbuf[i].handle)
			continue;
		destroy_drm_dumb_bo(drm_fd, drmbuf[i].handle);
		drmbuf[i].handle = 0;
		drmbuf[i].pitch = 0;
		drmbuf[i].size = 0;
	}

	if (-1 != drm_fd) {
		close(drm_fd);
		drm_fd = -1;
	}

	if (EGL_NO_CONTEXT != context) {
		eglMakeCurrent(display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
		eglDestroyContext(display, context);
		context = EGL_NO_CONTEXT;
	}

	if (EGL_NO_DISPLAY != display) {
		eglTerminate(display);
		display = EGL_NO_DISPLAY;
	}
}

static bool
reportGLCaps()
{
	const GLubyte* str_version	= glGetString(GL_VERSION);
	const GLubyte* str_vendor	= glGetString(GL_VENDOR);
	const GLubyte* str_renderer	= glGetString(GL_RENDERER);
	const GLubyte* str_glsl_ver	= glGetString(GL_SHADING_LANGUAGE_VERSION);
	const GLubyte* str_exten	= glGetString(GL_EXTENSIONS);

	stream::cout << "gl version, vendor, renderer, glsl version, extensions:"
		"\n\t" << (const char*) str_version <<
		"\n\t" << (const char*) str_vendor <<
		"\n\t" << (const char*) str_renderer <<
		"\n\t" << (const char*) str_glsl_ver <<
		"\n\t" << (const char*) str_exten << "\n\n";

	GLint params[2]; // we won't need more than 2

	glGetIntegerv(GL_MAX_TEXTURE_SIZE, params);
	stream::cout << "GL_MAX_TEXTURE_SIZE: " << params[0] << '\n';

	glGetIntegerv(GL_MAX_CUBE_MAP_TEXTURE_SIZE, params);
	stream::cout << "GL_MAX_CUBE_MAP_TEXTURE_SIZE: " << params[0] << '\n';

	glGetIntegerv(GL_MAX_VIEWPORT_DIMS, params);
	stream::cout << "GL_MAX_VIEWPORT_DIMS: " << params[0] << ", " << params[1] << '\n';

	glGetIntegerv(GL_MAX_RENDERBUFFER_SIZE, params);
	stream::cout << "GL_MAX_RENDERBUFFER_SIZE: " << params[0] << '\n';

	glGetIntegerv(GL_MAX_VERTEX_ATTRIBS, params);
	stream::cout << "GL_MAX_VERTEX_ATTRIBS: " << params[0] << '\n';

	glGetIntegerv(GL_MAX_VERTEX_UNIFORM_VECTORS, params);
	stream::cout << "GL_MAX_VERTEX_UNIFORM_VECTORS: " << params[0] << '\n';

	glGetIntegerv(GL_MAX_VARYING_VECTORS, params);
	stream::cout << "GL_MAX_VARYING_VECTORS: " << params[0] << '\n';

	glGetIntegerv(GL_MAX_FRAGMENT_UNIFORM_VECTORS, params);
	stream::cout << "GL_MAX_FRAGMENT_UNIFORM_VECTORS: " << params[0] << '\n';

	glGetIntegerv(GL_MAX_COMBINED_TEXTURE_IMAGE_UNITS, params);
	stream::cout << "GL_MAX_COMBINED_TEXTURE_IMAGE_UNITS: " << params[0] << '\n';

	glGetIntegerv(GL_MAX_VERTEX_TEXTURE_IMAGE_UNITS, params);
	stream::cout << "GL_MAX_VERTEX_TEXTURE_IMAGE_UNITS: " << params[0] << '\n';

	glGetIntegerv(GL_MAX_TEXTURE_IMAGE_UNITS, params);
	stream::cout << "GL_MAX_TEXTURE_IMAGE_UNITS: " << params[0] << '\n';

	stream::cout << '\n';

	return true;
}

static bool
validate_fullscreen(
	const char* string_w,
	const char* string_h,
	const char* string_hz,
	unsigned &screen_w,
	unsigned &screen_h)
{
	if (NULL == string_w || NULL == string_h || NULL == string_hz)
		return false;

	unsigned w, h, hz;

	if (1 != sscanf(string_w, "%u", &w))
		return false;

	if (1 != sscanf(string_h, "%u", &h))
		return false;

	if (1 != sscanf(string_hz, "%u", &hz))
		return false;

	if (!w || !h || !hz)
		return false;

	screen_w = w;
	screen_h = h;

	return true;
}

static bool
validate_bitness(
	const char* string_r,
	const char* string_g,
	const char* string_b,
	const char* string_a,
	unsigned (&screen_bitness)[4])
{
	if (NULL == string_r || NULL == string_g || NULL == string_b || NULL == string_a)
		return false;

	unsigned bitness[4];

	if (1 != sscanf(string_r, "%u", &bitness[0]))
		return false;

	if (1 != sscanf(string_g, "%u", &bitness[1]))
		return false;

	if (1 != sscanf(string_b, "%u", &bitness[2]))
		return false;

	if (1 != sscanf(string_a, "%u", &bitness[3]))
		return false;

	if (!bitness[0] || 16 < bitness[0] ||
		!bitness[1] || 16 < bitness[1] ||
		!bitness[2] || 16 < bitness[2] ||
		16 < bitness[3])
	{
		return false;
	}

	screen_bitness[0] = bitness[0];
	screen_bitness[1] = bitness[1];
	screen_bitness[2] = bitness[2];
	screen_bitness[3] = bitness[3];

	return true;
}

struct Param {
	enum {
		FLAG_PRINT_PERF_COUNTERS = 1
	};
	unsigned flags;         // combination of flags
	unsigned image_w;       // frame width
	unsigned image_h;       // frame height
	unsigned bitness[4];    // rgba bitness
	unsigned fsaa;          // fsaa number of samples
	unsigned frames;        // frames to run
	unsigned drawcalls;     // repeated drawcalls
};

static int
parse_cli(const int argc, char** const argv, struct Param& param)
{
	bool cli_err = false;
	bool cli_app = false;

	const unsigned prefix_len = strlen(util::arg_prefix);

	for (int i = 1; i < argc && !cli_err; ++i) {
		if (strncmp(argv[i], util::arg_prefix, prefix_len)) {
			cli_err = !cli_app;
			continue;
		}

		if (!strcmp(argv[i] + prefix_len, util::arg_app)) {
			if (++i == argc)
				cli_err = true;

			cli_app = true;
			continue;
		}

		cli_app = false;

		if (i + 1 < argc && !strcmp(argv[i] + prefix_len, arg_nframes)) {
			if (1 != sscanf(argv[i + 1], "%u", &param.frames))
				cli_err = true;

			i += 1;
			continue;
		}

		if (i + 3 < argc && !strcmp(argv[i] + prefix_len, arg_screen)) {
			if (!validate_fullscreen(argv[i + 1], argv[i + 2], argv[i + 3], param.image_w, param.image_h))
				cli_err = true;

			i += 3;
			continue;
		}

		if (i + 4 < argc && !strcmp(argv[i] + prefix_len, arg_bitness)) {
			if (!validate_bitness(argv[i + 1], argv[i + 2], argv[i + 3], argv[i + 4], param.bitness))
				cli_err = true;

			i += 4;
			continue;
		}

		if (i + 1 < argc && !strcmp(argv[i] + prefix_len, arg_fsaa)) {
			if (1 != sscanf(argv[i + 1], "%u", &param.fsaa))
				cli_err = true;

			i += 1;
			continue;
		}

		if (i + 1 < argc && !strcmp(argv[i] + prefix_len, arg_drawcalls)) {
			if (1 != sscanf(argv[i + 1], "%u", &param.drawcalls) || !param.drawcalls)
				cli_err = true;

			i += 1;
			continue;
		}

		cli_err = true;
	}

	if (cli_err) {
		stream::cerr << "usage: " << argv[0] << " [<option> ...]\n"
			"options:\n"
			"\t" << util::arg_prefix << arg_nframes <<
			" <unsigned_integer>\t\t: set number of frames to run; default is max unsigned int\n"
			"\t" << util::arg_prefix << arg_screen <<
			" <width> <height> <Hz>\t\t: set fullscreen output of specified geometry and refresh\n"
			"\t" << util::arg_prefix << arg_bitness <<
			" <r> <g> <b> <a>\t\t: set EGL config of specified RGBA bitness; default is screen's bitness\n"
			"\t" << util::arg_prefix << arg_fsaa <<
			" <positive_integer>\t\t: set fullscreen antialiasing; default is none\n"
			"\t" << util::arg_prefix << arg_drawcalls <<
			" <positive_integer>\t\t: set number of drawcalls per frame; may be ignored by apps\n"
			"\t" << util::arg_prefix << util::arg_app <<
			" <option> [<arguments>]\t\t: app-specific option" << '\n';

		return 1;
	}

	return cli_err;
}

int main(
	int argc,
	char** argv)
{
	using util::scoped_linkage_ptr;
	using util::scoped_functor;
	using util::deinit_resources_t;

	stream::cin.open(stdin);
	stream::cout.open(stdout);
	stream::cerr.open(stderr);

	Param param;
	param.flags = 0;
	param.image_w = 512;
	param.image_h = 512;
	param.bitness[0] = 8;
	param.bitness[1] = 8;
	param.bitness[2] = 8;
	param.bitness[3] = 8;
	param.fsaa = 0;
	param.frames = -1U;
	param.drawcalls = 0;

	if (parse_cli(argc, argv, param))
		return EXIT_FAILURE;

	const unsigned image_w         = param.image_w;
	const unsigned image_h         = param.image_h;
	const unsigned (& bitness)[4]  = param.bitness;
	const unsigned fsaa            = param.fsaa;
	const unsigned frames          = param.frames;
	const unsigned drawcalls       = param.drawcalls;

	if (drawcalls && !hook::set_num_drawcalls(drawcalls))
		stream::cerr << "drawcalls argument ignored by app\n";

	// set up GLES
	EGL egl(image_w, image_h);

	if (!egl.initGLES2(fsaa, bitness)) {
		stream::cerr << "Error initializing GLES\n";
		return EXIT_FAILURE;
	}

	if (!reportGLCaps()) {
		stream::cerr << "Error reporting GLES caps\n";
		return EXIT_FAILURE;
	}

	if (!hook::init_resources(argc, argv)) {
		stream::cerr << "Error initializing app resources\n";
		return EXIT_FAILURE;
	}

	const scoped_linkage_ptr< deinit_resources_t, scoped_functor, hook::deinit_resources > on_exit;

	if (0 == frames)
		return EXIT_SUCCESS;

	// create a dummy fence sync for buffer1
	if (egl.create_fence_sync(1)) {
		stream::cerr << "Error creating fence sync\n";
		return EXIT_FAILURE;
	}

	// set up wayland
	wl_display *display = setup_wayland();

	if (!display) {
		stream::cerr << "Error opening wayland display\n";
		return EXIT_FAILURE;
	}

	wl_shell_surface *shell_surface = create_surface(&egl);

	for (size_t i = 0; i < countof(egl.buffer); ++i) {
		egl.buffer[i] = create_buffer_dmabuf(
			egl.drmbuf[i].dmabuf_fd,
			egl.drmbuf[i].pitch,
			egl.fourcc,
			image_w, image_h);
	}

	// make sure a frame listener is active before first surface commit
	set_frame_listener(&egl);

	const uint64_t t0 = timer_ns();

	wl_surface_attach(egl.surface, egl.buffer[0], 0, 0);
	wl_surface_commit(egl.surface);

	while (wl_display_dispatch(display) != -1) {
		// redraw iteration has resulted in an error -- break the dispatch loop
		if (egl.frame_error)
			break;

		// raise an artificial error to suppress next redraw and break after the next dispatch iteration
		if (egl.frame_count == frames)
			egl.frame_error = -1;
	}

	const uint64_t dt = timer_ns() - t0;
	stream::cout << "total frames rendered: " << egl.frame_count <<
		"\ndrawcalls per frame: " << hook::get_num_drawcalls() << '\n';

	if (dt) {
		const double sec = double(dt) * 1e-9;
		stream::cout << "elapsed time: " << sec << " s"
			"\naverage FPS: " << (double(egl.frame_count) / sec) << '\n';
	}

#if QUIRK0001_SYSTEM_CRASH_AT_EXIT
	// keep the dispatch loop going indefinitely to avoid system crash on RK3399 (ticket #4)
	while (wl_display_dispatch(display) != -1) {}

#endif
	// clean up wayland
	for (size_t i = 0; i < countof(egl.buffer); ++i) {
		wl_buffer_destroy(egl.buffer[i]);
	}

	wl_shell_surface_destroy(shell_surface);
	wl_surface_destroy(egl.surface);
	cleanup_wayland(display);

	return EXIT_SUCCESS;
}
