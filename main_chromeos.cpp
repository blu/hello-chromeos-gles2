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
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <wayland-client.h>
#include <drm_fourcc.h>
#include "linux-dmabuf-unstable-v1-client-protocol.h"

#include <string>
#include <sstream>
#include <iomanip>

#include <png.h>
#ifndef Z_BEST_COMPRESSION
#define Z_BEST_COMPRESSION 9
#endif

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
class generic_free
{
public:
	void operator()(T* arg) {
		free(arg);
	}
};

template <>
class scoped_functor< FILE >
{
public:
	void operator()(FILE* arg) {
		fclose(arg);
	}
};

} // namespace util

namespace { // anonymous

const char arg_nframes[]       = "frames";
const char arg_screen[]        = "screen";
const char arg_bitness[]       = "bitness";
const char arg_config_id[]     = "config_id";
const char arg_fsaa[]          = "fsaa";
const char arg_grab[]          = "grab_frame";
const char arg_drawcalls[]     = "drawcalls";
const char arg_print_configs[] = "print_egl_configs";

#define FULLSCREEN_WIDTH 1366
#define FULLSCREEN_HEIGHT 768

typedef uint32_t pixel_t;

wl_compositor *compositor;
wl_display *display;
wl_shell *shell;
wl_shm *shm;
wl_buffer *bufferW;
wl_buffer *bufferF;
zwp_linux_dmabuf_v1 *dmabuf;

bool frame_error;
uint32_t frames_done;
int32_t curr_width;
int32_t curr_height;

void bind_buffer(wl_buffer *buffer, wl_shell_surface *shell_surface);

void registry_global(void *data, wl_registry *registry, uint32_t name, const char *interface, uint32_t version);
void registry_global_remove(void *, wl_registry *, uint32_t);

void redraw(void *, wl_callback *, uint32_t);

const wl_registry_listener registry_listener = {
	.global = registry_global,
	.global_remove = registry_global_remove
};

int setup_wayland(void)
{
	wl_registry *registry;

	display = wl_display_connect(NULL);
	if (display == NULL)
		return -1;

	registry = wl_display_get_registry(display);
	wl_registry_add_listener(registry, &registry_listener, NULL);
	wl_display_roundtrip(display);
	wl_registry_destroy(registry);

	return 0;
}

void cleanup_wayland(void)
{
	wl_shell_destroy(shell);
	wl_shm_destroy(shm);
	wl_compositor_destroy(compositor);
	wl_display_disconnect(display);
}

static void
dmabuf_format(void *data, struct zwp_linux_dmabuf_v1 *zwp_linux_dmabuf, uint32_t format) {
	fprintf(stderr, "%s, 0x%08x, %c%c%c%c\n", __FUNCTION__, format,
		char(format >>  0),
		char(format >>  8),
		char(format >> 16),
		char(format >> 24));
}

static void
dmabuf_modifier(void *data, struct zwp_linux_dmabuf_v1 *zwp_linux_dmabuf,
	uint32_t format, uint32_t modifier_hi, uint32_t modifier_lo) {
}

static const struct zwp_linux_dmabuf_v1_listener dmabuf_listener = {
    .format = dmabuf_format, // up to protocol version 2 zwp_linux_dmabuf_v1_listener::format is called back
    .modifier = dmabuf_modifier // from protocol version 3 onwards zwp_linux_dmabuf_v1_listener::modifier is called back
};

void registry_global(void *data, wl_registry *registry, uint32_t name, const char *interface, uint32_t version)
{
#if DEBUG
    fprintf(stderr, "%s, %s id %d version %d\n", __FUNCTION__, interface, name, version);

#endif
	if (strcmp(interface, wl_compositor_interface.name) == 0)
		compositor = static_cast< wl_compositor* >(wl_registry_bind(registry, name, &wl_compositor_interface, 3));
	else if (strcmp(interface, wl_shm_interface.name) == 0)
		shm = static_cast< wl_shm* >(wl_registry_bind(registry, name, &wl_shm_interface, 1));
	else if (strcmp(interface, zwp_linux_dmabuf_v1_interface.name) == 0) {
		dmabuf = static_cast< zwp_linux_dmabuf_v1* >(wl_registry_bind(registry, name, &zwp_linux_dmabuf_v1_interface, 2));
		zwp_linux_dmabuf_v1_add_listener(dmabuf, &dmabuf_listener, data);
	}
	else if (strcmp(interface, wl_shell_interface.name) == 0)
		shell = static_cast< wl_shell* >(wl_registry_bind(registry, name, &wl_shell_interface, 1));
}

void registry_global_remove(void *, wl_registry *, uint32_t) {
}

struct pool_data {
	int fd;
	pixel_t *memory;
	unsigned capacity;
	unsigned size;
};

wl_shm_pool *create_memory_pool(int file)
{
	pool_data *data;
	wl_shm_pool *pool;
	struct stat stat;

	if (fstat(file, &stat) != 0)
		return NULL;

	data = static_cast< pool_data* >(malloc(sizeof(*data)));

	if (data == NULL)
		return NULL;

	data->capacity = stat.st_size;
	data->size = 0;
	data->fd = file;

	data->memory = static_cast< pixel_t* >(mmap(0, data->capacity,
		PROT_READ | PROT_WRITE, MAP_SHARED, data->fd, 0));

	if (data->memory == MAP_FAILED)
		goto cleanup_alloc;

	pool = wl_shm_create_pool(shm, data->fd, data->capacity);

	if (pool == NULL)
		goto cleanup_mmap;

	wl_shm_pool_set_user_data(pool, data);

	return pool;

cleanup_mmap:
	munmap(data->memory, data->capacity);

cleanup_alloc:
	free(data);
	return NULL;
}

void free_memory_pool(wl_shm_pool *pool)
{
	pool_data *data;

	data = static_cast< pool_data* >(wl_shm_pool_get_user_data(pool));
	wl_shm_pool_destroy(pool);
	munmap(data->memory, data->capacity);
	free(data);
}

wl_buffer *create_buffer(wl_shm_pool *pool, unsigned width, unsigned height)
{
	pool_data *data;
	wl_buffer *buffer;

	data = static_cast< pool_data* >(wl_shm_pool_get_user_data(pool));
	if (data->capacity < data->size + width * height * sizeof(pixel_t))
		return NULL;

	buffer = wl_shm_pool_create_buffer(pool, data->size, width, height,
		width * sizeof(pixel_t), WL_SHM_FORMAT_ABGR8888);

	if (buffer == NULL)
		return NULL;

	wl_buffer_set_user_data(buffer, (void*)((uintptr_t) data->memory + data->size));
	data->size += width * height * sizeof(pixel_t);

	return buffer;
}

void free_buffer(wl_buffer *buffer)
{
	wl_buffer_destroy(buffer);
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
	// handle both scenarios symmetrically by binding the corresponding buffer at first call
	const int was_fullscreen = FULLSCREEN_WIDTH == curr_width && FULLSCREEN_HEIGHT == curr_height;
	const int now_fullscreen = FULLSCREEN_WIDTH == width && FULLSCREEN_HEIGHT == height;

	curr_width = width;
	curr_height = height;

	switch (was_fullscreen * 2 + now_fullscreen) {
	case 1: // was_fullscreen: 0, now_fullscreen: 1
		bind_buffer(bufferF, shell_surface);
		break;
	case 2: // was_fullscreen: 1, now_fullscreen: 0
		bind_buffer(bufferW, shell_surface);
		break;
	}
}

const wl_shell_surface_listener shell_surface_listener = {
	.ping = shell_surface_ping,
	.configure = shell_surface_configure,
};

const wl_callback_listener frame_listener = {
	.done = redraw
};

void set_frame_listener(wl_surface *surface)
{
	wl_callback *callback = wl_surface_frame(surface);
	wl_callback_add_listener(callback, &frame_listener, surface);
}

void redraw(void *data, wl_callback *callback, uint32_t time)
{
	// a callback is triggered only once and then abandoned; destroy the old
	// callback that invoked us so it doesn't leak, then create a new callback
	wl_callback_destroy(callback);

	wl_surface *surface = static_cast< wl_surface* >(data);
	set_frame_listener(surface);

	// copy a frame from the EGL surface into the wayland surface buffer; note
	// that glReadPixels is a serializing op that will wait for GLES to finish
	void *pixels = wl_buffer_get_user_data(bufferW);
	glReadPixels(0, 0, curr_width, curr_height, GL_RGBA, GL_UNSIGNED_BYTE, pixels);

	// update wayland surface and trigger the corresponding screen update
	wl_surface_attach(surface, bufferW, 0, 0);
	wl_surface_damage(surface, 0, 0, curr_width, curr_height);
	wl_surface_commit(surface);

	// emit app's next frame
	frame_error = !hook::render_frame();
	frames_done += 1;
}

wl_shell_surface *create_surface(void)
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

	wl_shell_surface_add_listener(shell_surface, &shell_surface_listener, 0);
	wl_shell_surface_set_toplevel(shell_surface);
	wl_shell_surface_set_user_data(shell_surface, surface);
	wl_surface_set_user_data(surface, NULL);

	return shell_surface;
}

void free_surface(wl_shell_surface *shell_surface)
{
	wl_surface *surface = static_cast< wl_surface* >(wl_shell_surface_get_user_data(shell_surface));

	wl_shell_surface_destroy(shell_surface);
	wl_surface_destroy(surface);
}

void bind_buffer(wl_buffer *buffer, wl_shell_surface *shell_surface)
{
	wl_surface *surface = static_cast< wl_surface* >(wl_shell_surface_get_user_data(shell_surface));

	wl_surface_attach(surface, buffer, 0, 0);
	wl_surface_commit(surface);
}

int set_cloexec_or_close(int fd)
{
	long flags;

	if (fd == -1)
		return -1;

	flags = fcntl(fd, F_GETFD);

	if (flags == -1)
		goto err;

	if (fcntl(fd, F_SETFD, flags | FD_CLOEXEC) == -1)
		goto err;

	return fd;

err:
	close(fd);
	return -1;
}

int create_tmpfile_cloexec(char *tmpname)
{
	int fd;

	fd = mkstemp(tmpname);

	if (fd >= 0) {
		fd = set_cloexec_or_close(fd);
		unlink(tmpname);
	}

	return fd;
}

/*
 * Create a new, unique, anonymous file of the given size, and
 * return the file descriptor for it. The file descriptor is set
 * CLOEXEC. The file is immediately suitable for mmap()'ing
 * the given size at offset zero.
 *
 * The file should not have a permanent backing store like a disk,
 * but may have if XDG_RUNTIME_DIR is not properly implemented in OS.
 *
 * The file name is deleted from the file system.
 *
 * The file is suitable for buffer sharing between processes by
 * transmitting the file descriptor over Unix sockets using the
 * SCM_RIGHTS methods.
 */
int
os_create_anonymous_file(off_t size)
{
	const char templat[] = "/egl-shared-XXXXXX";
	const char *path;
	char *name;
	int fd;

	path = getenv("XDG_RUNTIME_DIR");
	if (!path) {
		errno = ENOENT;
		return -1;
	}

	name = static_cast< char* >(malloc(strlen(path) + sizeof(templat)));

	if (!name)
		return -1;

	strcpy(name, path);
	strcat(name, templat);

	fd = create_tmpfile_cloexec(name);

	free(name);

	if (fd < 0)
		return -1;

	if (ftruncate(fd, size) < 0) {
		close(fd);
		return -1;
	}

	return fd;
}

} // namespace anonymous

////////////////////////////////////////////////////////////////////////////////////////////////////
// EGL helper
////////////////////////////////////////////////////////////////////////////////////////////////////

struct DRMBuffer {
	// DRM dumb-buffer-object properties
	uint32_t handle;
	uint32_t pitch;
	uint64_t size;

	// buffer-object PRIME file descriptor
	int32_t dmabuf_fd;

	DRMBuffer()
	: handle(0)
	, pitch(0)
	, size(0)
	, dmabuf_fd(-1)
	{
	}
};

struct EGL
{
	EGLDisplay display;
	EGLContext context;
	EGLImage image;

	uint32_t window_w;
	uint32_t window_h;

	int32_t drm_fd;
	DRMBuffer imageDRM;

	EGL(uint32_t w,
		uint32_t h)
	: display(EGL_NO_DISPLAY)
	, context(EGL_NO_CONTEXT)
	, image(EGL_NO_IMAGE_KHR)
	, window_w(w)
	, window_h(h)
	, drm_fd(-1)
	{
		init_egl_ext();
		init_gles_ext();
	}

	bool initGLES2(
		const unsigned config_id,
		const unsigned fsaa,
		const unsigned (& bitness)[4],
		const bool print_configs);

	void deinit();
};

static std::string
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

static std::string
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


static std::string
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
	void operator()(EGL* arg) {
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
	uint64_t *const size) {

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
	const uint32_t handle) {

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

	if (ret || prime_request.fd < 0)
		return ret;

	*dmabuf_fd = prime_request.fd;
	return 0;
}

bool EGL::initGLES2(
	const unsigned config_id,
	const unsigned fsaa,
	const unsigned (& bitness)[4],
	const bool print_configs)
{
	using util::scoped_ptr;
	using util::scoped_functor;

	const unsigned nbits_r = bitness[0];
	const unsigned nbits_g = bitness[1];
	const unsigned nbits_b = bitness[2];
	const unsigned nbits_a = bitness[3];
	const unsigned nbits_pixel =
		nbits_r +
		nbits_g +
		nbits_b +
		nbits_a + 7 & ~7;

	if (0 == config_id && 0 == nbits_pixel) {
		stream::cerr << "nil pixel size requested; abort\n";
		return false;
	}

	deinit();

	uint32_t fourcc = 0;
	switch (nbits_r << 24 | nbits_g << 16 | nbits_b << 8 | nbits_a) {
	case 0x08080808:
		fourcc = DRM_FORMAT_ARGB8888;
		break;
	case 0x08080800:
		fourcc = DRM_FORMAT_XRGB8888;
		break;
	case 0x05060500:
		fourcc = DRM_FORMAT_RGB565;
		break;
	}

	if (0 == fourcc) {
		stream::cerr << "unsupported pixel format requested; abort\n";
		return false;
	}

	drm_fd = open("/dev/dri/vgem", O_RDWR | O_CLOEXEC);

	if (drm_fd < 0) {
		stream::cerr << "Could not open /dev/dri/vgem: " << strerror(errno) << '\n';
		return false;
	}

	int ret = create_drm_dumb_bo(
		drm_fd, window_w, window_h, nbits_pixel,
		&imageDRM.handle,
		&imageDRM.pitch,
		&imageDRM.size);

	if (ret) {
		stream::cerr << "Could not allocate dumb buffer: " << strerror(ret) << " (" << ret << ")\n";
		return false;
	}

	ret = create_drm_dmabuf(drm_fd, imageDRM.handle, &imageDRM.dmabuf_fd);

	if (ret) {
		stream::cerr << "Could not export buffer: " << strerror(ret) << " (" << ret << "), handle: " << imageDRM.handle << '\n';
		destroy_drm_dumb_bo(drm_fd, imageDRM.handle);
		return false;
	}

	display = eglGetDisplay(EGL_DEFAULT_DISPLAY);

	scoped_ptr< EGL, scoped_functor > deinit_self(this);

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

	const EGLint image_attr[] = {
		EGL_WIDTH,
		EGLint(window_w),
		EGL_HEIGHT,
		EGLint(window_h),
		EGL_LINUX_DRM_FOURCC_EXT,
		EGLint(fourcc),
		EGL_DMA_BUF_PLANE0_FD_EXT,
		EGLint(imageDRM.dmabuf_fd),
		EGL_DMA_BUF_PLANE0_OFFSET_EXT,
		EGLint(0),
		EGL_DMA_BUF_PLANE0_PITCH_EXT,
		EGLint(imageDRM.pitch)
	};
	image = eglCreateImageKHR(
		display,
		EGL_NO_CONTEXT,
		EGL_LINUX_DMA_BUF_EXT,
		EGLClientBuffer(NULL),
		image_attr);

	if (EGL_NO_IMAGE_KHR == image) {
		stream::cerr << "eglCreateImage() failed\n";
		return false;
	}

	if (EGL_FALSE == eglMakeCurrent(
			display,
			EGL_NO_SURFACE,
			EGL_NO_SURFACE,
			context)) {

		stream::cerr << "eglMakeCurrent() failed\n";
		return false;
	}

	// TODO: set the newly-acquired EGLimage as storage to our primary renderbuffer

	deinit_self.reset();
	return true;
}


void EGL::deinit()
{
	if (EGL_NO_IMAGE_KHR != image) {
		eglDestroyImage(display, image);
		image = EGL_NO_IMAGE_KHR;
	}

	if (-1 != imageDRM.dmabuf_fd) {
		close(imageDRM.dmabuf_fd);
		imageDRM.dmabuf_fd = -1;
	}

	if (0 != imageDRM.handle) {
		destroy_drm_dumb_bo(drm_fd, imageDRM.handle);
		imageDRM.handle = 0;
		imageDRM.pitch = 0;
		imageDRM.size = 0;
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

enum IMAGE_FORMAT {
	IMAGE_FORMAT_GRAY,
	IMAGE_FORMAT_RGB,
	IMAGE_FORMAT_RGBX
};

static bool
write_png(
	const IMAGE_FORMAT src_format,
	const unsigned w,
	const unsigned h,
	void* const bits,
	FILE* const fp)
{
	using util::scoped_ptr;
	using util::generic_free;

	size_t pixel_size;
	int color_type;

	switch (src_format) {
	case IMAGE_FORMAT_GRAY:
		pixel_size = sizeof(png_byte);
		color_type = PNG_COLOR_TYPE_GRAY;
		break;
	case IMAGE_FORMAT_RGB:
		pixel_size = sizeof(png_byte[3]);
		color_type = PNG_COLOR_TYPE_RGB;
		break;
	case IMAGE_FORMAT_RGBX:
		pixel_size = sizeof(png_byte[4]);
		color_type = PNG_COLOR_TYPE_RGB;
		break;
	default: // unknown src format
		return false;
	}

	png_structp png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);

	if (!png_ptr)
		return false;

	png_infop info_ptr = png_create_info_struct(png_ptr);

	if (!info_ptr) {
		png_destroy_write_struct(&png_ptr, (png_infopp) NULL);
		return false;
	}

	// declare any RAII before the longjump, lest no destruction at longjump
	const scoped_ptr< png_bytep, generic_free > row((png_bytepp) malloc(sizeof(png_bytep) * h));

	if (setjmp(png_jmpbuf(png_ptr))) {
		png_destroy_write_struct(&png_ptr, &info_ptr);
		return false;
	}

	for (size_t i = 0; i < h; ++i)
		row()[i] = (png_bytep) bits + w * (h - 1 - i) * pixel_size;

	png_init_io(png_ptr, fp);
	png_set_compression_level(png_ptr, Z_BEST_COMPRESSION);
	png_set_IHDR(png_ptr, info_ptr, w, h, 8, color_type, PNG_INTERLACE_NONE, PNG_COMPRESSION_TYPE_DEFAULT, PNG_FILTER_TYPE_DEFAULT);
	png_write_info(png_ptr, info_ptr);
	if (IMAGE_FORMAT_RGBX == src_format)
		png_set_filler(png_ptr, 0, PNG_FILLER_AFTER);
	png_write_image(png_ptr, row());
	png_write_end(png_ptr, info_ptr);

	png_destroy_write_struct(&png_ptr, &info_ptr);
	return true;
}


static bool
saveFramebuffer(
	const unsigned window_w,
	const unsigned window_h,
	const char* name)
{
	assert(NULL != name);

	using util::scoped_ptr;
	using util::generic_free;
	using util::scoped_functor;

	stream::cout << "saving framegrab as '" << name << "'\n";

	scoped_ptr< FILE, scoped_functor > file(fopen(name, "wb"));

	if (0 == file()) {
		stream::cerr << "failure opening framegrab file '" << name << "'\n";
		return false;
	}

	const size_t pixel_size = sizeof(GLubyte[4]);
	const size_t frame_size = window_h * window_w * pixel_size;
	const scoped_ptr< GLvoid, generic_free > pixels(malloc(frame_size));

	glReadPixels(0, 0, window_w, window_h, GL_RGBA, GL_UNSIGNED_BYTE, pixels());

	if (!write_png(IMAGE_FORMAT_RGBX, window_w, window_h, pixels(), file())) {
		stream::cerr << "failure writing framegrab file '" << name << "'\n";
		return false;
	}

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
		FLAG_PRINT_CONFIGS       = 1,
		FLAG_PRINT_PERF_COUNTERS = 2
	};
	unsigned flags;         // combination of flags
	unsigned image_w;       // frame width
	unsigned image_h;       // frame height
	unsigned bitness[4];    // rgba bitness
	unsigned fsaa;          // fsaa number of samples
	unsigned frames;        // frames to run
	unsigned config_id;     // EGL config id
	unsigned drawcalls;     // repeated drawcalls
	unsigned grab_frame;    // frame to grab
	const char* grab_filename;
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

		if (i + 2 < argc && !strcmp(argv[i] + prefix_len, arg_grab)) {
			if (1 != sscanf(argv[i + 1], "%u", &param.grab_frame))
				cli_err = true;

			param.grab_filename = argv[i + 2];
			i += 2;
			continue;
		}

		if (i + 1 < argc && !strcmp(argv[i] + prefix_len, arg_drawcalls)) {
			if (1 != sscanf(argv[i + 1], "%u", &param.drawcalls) || !param.drawcalls)
				cli_err = true;

			i += 1;
			continue;
		}

		if (i + 1 < argc && !strcmp(argv[i] + prefix_len, arg_config_id)) {
			if (1 != sscanf(argv[i + 1], "%u", &param.config_id) || 0 == param.config_id)
				cli_err = true;

			i += 1;
			continue;
		}

		if (!strcmp(argv[i] + prefix_len, arg_print_configs)) {
			param.flags |= unsigned(Param::FLAG_PRINT_CONFIGS);
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
			"\t" << util::arg_prefix << arg_config_id <<
			" <positive_integer>\t\t: set EGL config of specified ID; overrides any other config options\n"
			"\t" << util::arg_prefix << arg_fsaa <<
			" <positive_integer>\t\t: set fullscreen antialiasing; default is none\n"
			"\t" << util::arg_prefix << arg_grab <<
			" <unsigned_integer> <file>\t: grab the Nth frame to file; index is zero-based\n"
			"\t" << util::arg_prefix << arg_drawcalls <<
			" <positive_integer>\t\t: set number of drawcalls per frame; may be ignored by apps\n"
			"\t" << util::arg_prefix << arg_print_configs <<
			"\t\t\t: print available EGL configs\n"
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
	stream::cin.open(stdin);
	stream::cout.open(stdout);
	stream::cerr.open(stderr);

	Param param;
	param.flags = 0;
	param.image_w = 512;
	param.image_h = 512;
	param.bitness[0] = 0;
	param.bitness[1] = 0;
	param.bitness[2] = 0;
	param.bitness[3] = 0;
	param.fsaa = 0;
	param.frames = -1U;
	param.config_id = 0;
	param.drawcalls = 0;
	param.grab_frame = -1U;
	param.grab_filename = NULL;

	if (parse_cli(argc, argv, param))
		return EXIT_FAILURE;

	const bool print_configs       = bool(param.flags & unsigned(Param::FLAG_PRINT_CONFIGS));
	const unsigned image_w         = param.image_w;
	const unsigned image_h         = param.image_h;
	const unsigned (& bitness)[4]  = param.bitness;
	const unsigned fsaa            = param.fsaa;
	const unsigned frames          = param.frames;
	const unsigned config_id       = param.config_id;
	const unsigned drawcalls       = param.drawcalls;
	const unsigned grab_frame      = param.grab_frame;
	const char* grab_filename      = param.grab_filename;

	if (drawcalls && !hook::set_num_drawcalls(drawcalls))
		stream::cerr << "drawcalls argument ignored by app\n";

	if (0 == config_id && 0 == bitness[0]) {
		stream::cerr << "unspecified framebuffer bitness; bailing out\n";
		return EXIT_FAILURE;
	}

	// set up GLES
	EGL egl(image_w, image_h);

	if (!egl.initGLES2(config_id, fsaa, bitness, print_configs) ||
		!reportGLCaps() ||
		!hook::init_resources(argc, argv))
	{
		stream::cerr << "failed to initialise either GLES or resources; bailing out\n";
		return EXIT_FAILURE;
	}

	if (0 == frames)
		return EXIT_SUCCESS;

	// emit app's first frame
	if (!hook::render_frame())
		return EXIT_SUCCESS;

	// set up wayland
	wl_shm_pool *pool;
	wl_shell_surface *shell_surface;
	const int memfile = os_create_anonymous_file((image_w * image_h + FULLSCREEN_WIDTH * FULLSCREEN_HEIGHT) * sizeof(pixel_t));

	if (memfile < 0) {
		fprintf(stderr, "Error opening memfile\n");
		return EXIT_FAILURE;
	}

	if (setup_wayland()) {
		fprintf(stderr, "Error opening display\n");
		return EXIT_FAILURE;
	}

	pool          = create_memory_pool(memfile);
	shell_surface = create_surface();
	bufferW       = create_buffer(pool, image_w, image_h);
	bufferF       = create_buffer(pool, FULLSCREEN_WIDTH, FULLSCREEN_HEIGHT);

	// start in windowed mode
	curr_width = image_w;
	curr_height = image_h;

	// next buffer bind below will trigger first commit, so make sure a frame listener is active
	wl_surface *surface = static_cast< wl_surface* >(wl_shell_surface_get_user_data(shell_surface));
	set_frame_listener(surface);

	const uint64_t t0 = timer_ns();

	bind_buffer(bufferW, shell_surface);

	while (wl_display_dispatch(display) != -1) {
		if (frame_error || frames_done == frames)
			break;
	}

	const uint64_t dt = timer_ns() - t0;
	stream::cout << "total frames rendered: " << frames_done <<
		"\ndrawcalls per frame: " << hook::get_num_drawcalls() << '\n';

	if (dt) {
		const double sec = double(dt) * 1e-9;
		stream::cout << "elapsed time: " << sec << " s"
			"\naverage FPS: " << (double(frames_done) / sec) << '\n';
	}

	// clean up wayland
	free_buffer(bufferF);
	free_buffer(bufferW);
	free_surface(shell_surface);
	free_memory_pool(pool);
	cleanup_wayland();
	close(memfile);

	// clean up gles
	hook::deinit_resources();
	egl.deinit();

	return EXIT_SUCCESS;
}
