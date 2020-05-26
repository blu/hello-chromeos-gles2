#if PLATFORM_GL
	#include <GL/gl.h>
	#include <GL/glext.h>
	#include "gles_gl_mapping.hpp"
#else
	#include <EGL/egl.h>
	#include <GLES2/gl2.h>
	#include <GLES2/gl2ext.h>
	#include "gles_ext.h"
#endif // PLATFORM_GL
#include <unistd.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>
#include "cmath_fix"
#include <string>
#include <sstream>

#include "scoped.hpp"
#include "vectsimd.hpp"
#include "rendIndexedTrilist.hpp"
#include "rendSkeleton.hpp"
#if 0
#include "util_tex.hpp"
#endif
#include "util_misc.hpp"
#include "pure_macro.hpp"
#include "stream.hpp"

#include "rendVertAttr.hpp"

using util::scoped_ptr;
using util::scoped_functor;
using util::deinit_resources_t;

namespace sk {

#define SETUP_VERTEX_ATTR_POINTERS_MASK	( \
		SETUP_VERTEX_ATTR_POINTERS_MASK_vertex)

#include "rendVertAttr_setupVertAttrPointers.hpp"
#undef SETUP_VERTEX_ATTR_POINTERS_MASK

struct Vertex {
	GLfloat pos[3];
};

} // namespace sk

namespace { // anonymous

const char arg_prefix[]    = "-";
const char arg_app[]       = "app";

const char arg_anim_step[] = "anim_step";
const char arg_mesh[]      = "mesh";

const char* g_mesh_filename = "asset/mesh/tetra.mesh";
float g_angle;
float g_angle_step = .0125f;
simd::matx4 g_matx_fit;

} // namespace

namespace anim {

} // namespace anim

namespace { // anonymous

#if PLATFORM_EGL
EGLDisplay g_display = EGL_NO_DISPLAY;
EGLContext g_context = EGL_NO_CONTEXT;

#endif
enum {
	TEX__DUMMY, // avoid zero-sized declarations

	TEX_COUNT,
	TEX_FORCE_UINT = -1U
};

enum {
	PROG_SKIN,

	PROG_COUNT,
	PROG_FORCE_UINT = -1U
};

enum {
	UNI_LP_OBJ,
	UNI_VP_OBJ,
	UNI_MVP,

	UNI_COUNT,
	UNI_FORCE_UINT = -1U
};

enum {
	MESH_SKIN,

	MESH_COUNT,
	MESH_FORCE_UINT = -1U
};

enum {
	VBO_SKIN_VTX,
	VBO_SKIN_IDX,

	VBO_COUNT,
	VBO_FORCE_UINT = -1U
};

GLint g_uni[PROG_COUNT][UNI_COUNT];

#if PLATFORM_GL_OES_vertex_array_object
GLuint g_vao[PROG_COUNT];

#endif
GLuint g_tex[TEX_COUNT];
GLuint g_vbo[VBO_COUNT];
GLuint g_shader_vert[PROG_COUNT];
GLuint g_shader_frag[PROG_COUNT];
GLuint g_shader_prog[PROG_COUNT];

unsigned g_num_faces[MESH_COUNT];
GLenum g_index_type;

rend::ActiveAttrSemantics g_active_attr_semantics[PROG_COUNT];

} // namespace

bool hook::set_num_drawcalls(
	const unsigned)
{
	return false;
}

unsigned hook::get_num_drawcalls()
{
	return 1;
}

bool hook::requires_depth()
{
	return true;
}

static bool parse_cli(
	const unsigned argc,
	const char* const* argv)
{
	bool cli_err = false;
	const unsigned prefix_len = strlen(arg_prefix);

	for (unsigned i = 1; i < argc && !cli_err; ++i) {
		if (strncmp(argv[i], arg_prefix, prefix_len) ||
			strcmp(argv[i] + prefix_len, arg_app)) {
			continue;
		}

		if (++i < argc) {
			if (i + 1 < argc && !strcmp(argv[i], arg_mesh)) {
				g_mesh_filename = argv[i + 1];
				i += 1;
				continue;
			}
			else
			if (i + 1 < argc && !strcmp(argv[i], arg_anim_step)) {
				if (1 == sscanf(argv[i + 1], "%f", &g_angle_step) && 0.f < g_angle_step) {
					i += 1;
					continue;
				}
			}
		}

		cli_err = true;
	}

	if (cli_err) {
		stream::cerr << "app options:\n"
			"\t" << arg_prefix << arg_app << " " << arg_mesh <<
			" <filename>\t\t\t\t: use specified mesh file of coordinates and indices\n"
			"\t" << arg_prefix << arg_app << " " << arg_anim_step <<
			" <step>\t\t\t\t: use specified animation step; entire animation is 1.0\n\n";
	}

	return !cli_err;
}

namespace { // anonymous

template < unsigned PROG_T >
inline bool
bindVertexBuffersAndPointers()
{
	assert(false);
	return false;
}

template <>
inline bool
bindVertexBuffersAndPointers< PROG_SKIN >()
{
	glBindBuffer(GL_ARRAY_BUFFER, g_vbo[VBO_SKIN_VTX]);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, g_vbo[VBO_SKIN_IDX]);

	DEBUG_GL_ERR()

	return sk::setupVertexAttrPointers< sk::Vertex >(g_active_attr_semantics[PROG_SKIN]);
}

bool check_context(
	const char* prefix)
{
	bool context_correct = true;

#if PLATFORM_EGL
	if (g_display != eglGetCurrentDisplay()) {
		stream::cerr << prefix << " encountered foreign display\n";

		context_correct = false;
	}

	if (g_context != eglGetCurrentContext()) {
		stream::cerr << prefix << " encountered foreign context\n";

		context_correct = false;
	}

#endif
	return context_correct;
}

} // namespace

bool
hook::deinit_resources()
{
	if (!check_context(__FUNCTION__))
		return false;

	for (unsigned i = 0; i < sizeof(g_shader_prog) / sizeof(g_shader_prog[0]); ++i) {
		glDeleteProgram(g_shader_prog[i]);
		g_shader_prog[i] = 0;
	}

	for (unsigned i = 0; i < sizeof(g_shader_vert) / sizeof(g_shader_vert[0]); ++i) {
		glDeleteShader(g_shader_vert[i]);
		g_shader_vert[i] = 0;
	}

	for (unsigned i = 0; i < sizeof(g_shader_frag) / sizeof(g_shader_frag[0]); ++i) {
		glDeleteShader(g_shader_frag[i]);
		g_shader_frag[i] = 0;
	}

	glDeleteTextures(sizeof(g_tex) / sizeof(g_tex[0]), g_tex);
	memset(g_tex, 0, sizeof(g_tex));

#if PLATFORM_GL_OES_vertex_array_object
	glDeleteVertexArraysOES(sizeof(g_vao) / sizeof(g_vao[0]), g_vao);
	memset(g_vao, 0, sizeof(g_vao));

#endif
	glDeleteBuffers(sizeof(g_vbo) / sizeof(g_vbo[0]), g_vbo);
	memset(g_vbo, 0, sizeof(g_vbo));

#if PLATFORM_EGL
	g_display = EGL_NO_DISPLAY;
	g_context = EGL_NO_CONTEXT;

#endif
	return true;
}

namespace { // anonymous

#if DEBUG && PLATFORM_GL_KHR_debug
void debugProc(
	GLenum source,
	GLenum type,
	GLuint id,
	GLenum severity,
	GLsizei length,
	const GLchar* message,
	const void* userParam)
{
	fprintf(stderr, "log: %s\n", message);
}

#endif
} // namespace

bool
hook::init_resources(
	const unsigned argc,
	const char* const * argv)
{
	if (!parse_cli(argc, argv))
		return false;

#if DEBUG && PLATFORM_GL_KHR_debug
	glDebugMessageCallbackKHR(debugProc, NULL);
	glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS_KHR);
	glEnable(GL_DEBUG_OUTPUT_KHR);
	DEBUG_GL_ERR()

	glDebugMessageInsertKHR(
		GL_DEBUG_SOURCE_APPLICATION_KHR,
		GL_DEBUG_TYPE_OTHER_KHR,
		GLuint(42),
		GL_DEBUG_SEVERITY_HIGH_KHR,
		GLint(-1),
		"testing 1, 2, 3");
	DEBUG_GL_ERR()

#endif
#if PLATFORM_EGL
	g_display = eglGetCurrentDisplay();

	if (EGL_NO_DISPLAY == g_display) {
		stream::cerr << __FUNCTION__ << " encountered nil display\n";
		return false;
	}

	g_context = eglGetCurrentContext();

	if (EGL_NO_CONTEXT == g_context) {
		stream::cerr << __FUNCTION__ << " encountered nil context\n";
		return false;
	}

#endif
	scoped_ptr< deinit_resources_t, scoped_functor > on_error(deinit_resources);

	/////////////////////////////////////////////////////////////////

	glEnable(GL_CULL_FACE);

	const GLclampf red = .25f;
	const GLclampf green = .25f;
	const GLclampf blue = .25f;
	const GLclampf alpha = 1.f;

	glClearColor(red, green, blue, alpha);
	glClearDepthf(1.f);

	/////////////////////////////////////////////////////////////////

	glGenTextures(sizeof(g_tex) / sizeof(g_tex[0]), g_tex);

	for (unsigned i = 0; i < sizeof(g_tex) / sizeof(g_tex[0]); ++i)
		assert(g_tex[i]);

	/////////////////////////////////////////////////////////////////

	for (unsigned i = 0; i < PROG_COUNT; ++i)
		for (unsigned j = 0; j < UNI_COUNT; ++j)
			g_uni[i][j] = -1;

	/////////////////////////////////////////////////////////////////

	g_shader_vert[PROG_SKIN] = glCreateShader(GL_VERTEX_SHADER);
	assert(g_shader_vert[PROG_SKIN]);

	if (!util::setupShader(g_shader_vert[PROG_SKIN], "asset/shader/blinn.glslv")) {
		stream::cerr << __FUNCTION__ << " failed at setupShader\n";
		return false;
	}

	g_shader_frag[PROG_SKIN] = glCreateShader(GL_FRAGMENT_SHADER);
	assert(g_shader_frag[PROG_SKIN]);

	if (!util::setupShader(g_shader_frag[PROG_SKIN], "asset/shader/blinn.glslf")) {
		stream::cerr << __FUNCTION__ << " failed at setupShader\n";
		return false;
	}

	g_shader_prog[PROG_SKIN] = glCreateProgram();
	assert(g_shader_prog[PROG_SKIN]);

	if (!util::setupProgram(
			g_shader_prog[PROG_SKIN],
			g_shader_vert[PROG_SKIN],
			g_shader_frag[PROG_SKIN]))
	{
		stream::cerr << __FUNCTION__ << " failed at setupProgram\n";
		return false;
	}

	g_uni[PROG_SKIN][UNI_MVP]       = glGetUniformLocation(g_shader_prog[PROG_SKIN], "mvp");
	g_uni[PROG_SKIN][UNI_LP_OBJ]    = glGetUniformLocation(g_shader_prog[PROG_SKIN], "lp_obj");
	g_uni[PROG_SKIN][UNI_VP_OBJ]    = glGetUniformLocation(g_shader_prog[PROG_SKIN], "vp_obj");

	g_active_attr_semantics[PROG_SKIN].registerVertexAttr(glGetAttribLocation(g_shader_prog[PROG_SKIN], "at_Vertex"));

	/////////////////////////////////////////////////////////////////

#if PLATFORM_GL_OES_vertex_array_object
	glGenVertexArraysOES(sizeof(g_vao) / sizeof(g_vao[0]), g_vao);

	for (unsigned i = 0; i < sizeof(g_vao) / sizeof(g_vao[0]); ++i)
		assert(g_vao[i]);

#endif
	glGenBuffers(sizeof(g_vbo) / sizeof(g_vbo[0]), g_vbo);

	for (unsigned i = 0; i < sizeof(g_vbo) / sizeof(g_vbo[0]); ++i)
		assert(g_vbo[i]);

	float bbox_min[3];
	float bbox_max[3];

	if (!util::fill_indexed_trilist_from_file_P(
			g_mesh_filename,
			g_vbo[VBO_SKIN_VTX],
			g_vbo[VBO_SKIN_IDX],
			g_num_faces[MESH_SKIN],
			g_index_type,
			bbox_min,
			bbox_max))
	{
		stream::cerr << __FUNCTION__ << " failed at fill_indexed_trilist_from_file_P\n";
		return false;
	}

	const float centre[3] = {
		(bbox_min[0] + bbox_max[0]) * .5f,
		(bbox_min[1] + bbox_max[1]) * .5f,
		(bbox_min[2] + bbox_max[2]) * .5f
	};
	const float extent[3] = {
		(bbox_max[0] - bbox_min[0]) * .5f,
		(bbox_max[1] - bbox_min[1]) * .5f,
		(bbox_max[2] - bbox_min[2]) * .5f
	};
	const float rcp_extent = 1.f / fmaxf(fmaxf(extent[0], extent[1]), extent[2]);

	g_matx_fit = simd::matx4(
		rcp_extent,	0.f,		0.f,		0.f,
		0.f,		rcp_extent,	0.f,		0.f,
		0.f,		0.f,		rcp_extent,	0.f,
		-centre[0] * rcp_extent,
		-centre[1] * rcp_extent,
		-centre[2] * rcp_extent, 1.f);

#if PLATFORM_GL_OES_vertex_array_object
	glBindVertexArrayOES(g_vao[PROG_SKIN]);

	if (!bindVertexBuffersAndPointers< PROG_SKIN >() || (DEBUG_LITERAL && util::reportGLError())) {
		stream::cerr << __FUNCTION__ << "failed at bindVertexBuffersAndPointers for PROG_SKIN\n";
		return false;
	}

	for (unsigned i = 0; i < g_active_attr_semantics[PROG_SKIN].num_active_attr; ++i)
		glEnableVertexAttribArray(g_active_attr_semantics[PROG_SKIN].active_attr[i]);

	DEBUG_GL_ERR()

	glBindVertexArrayOES(0);

#endif // PLATFORM_GL_OES_vertex_array_object
	on_error.reset();
	return true;
}


namespace { // anonymous

class matx4_rotate : public simd::matx4 {
	matx4_rotate();
	matx4_rotate(const matx4_rotate&);

public:
	matx4_rotate(
		const float a,
		const float x,
		const float y,
		const float z)
	{
		const float sin_a = sinf(a);
		const float cos_a = cosf(a);

		static_cast< simd::matx4& >(*this) = simd::matx4(
			x * x + cos_a * (1 - x * x),         x * y - cos_a * (x * y) + sin_a * z, x * z - cos_a * (x * z) - sin_a * y, 0.f,
			y * x - cos_a * (y * x) - sin_a * z, y * y + cos_a * (1 - y * y),         y * z - cos_a * (y * z) + sin_a * x, 0.f,
			z * x - cos_a * (z * x) + sin_a * y, z * y - cos_a * (z * y) - sin_a * x, z * z + cos_a * (1 - z * z),         0.f,
			0.f,                                 0.f,                                 0.f,                                 1.f);
	}
};


class matx4_ortho : public simd::matx4
{
	matx4_ortho();
	matx4_ortho(const matx4_ortho&);

public:
	matx4_ortho(
		const float l,
		const float r,
		const float b,
		const float t,
		const float n,
		const float f)
	{
		static_cast< simd::matx4& >(*this) = simd::matx4(
			2.f / (r - l),     0.f,               0.f,               0.f,
			0.f,               2.f / (t - b),     0.f,               0.f,
			0.f,               0.f,               2.f / (n - f),     0.f,
			(r + l) / (l - r), (t + b) / (b - t), (f + n) / (n - f), 1.f);
	}
};


class matx4_persp : public simd::matx4
{
	matx4_persp();
	matx4_persp(const matx4_persp&);

public:
	matx4_persp(
		const float l,
		const float r,
		const float b,
		const float t,
		const float n,
		const float f)
	{
		static_cast< simd::matx4& >(*this) = simd::matx4(
			2.f * n / (r - l),  0.f,                0.f,                    0.f,
			0.f,                2.f * n / (t - b),  0.f,                    0.f,
			(r + l) / (r - l),  (t + b) / (t - b),  (f + n) / (n - f),     -1.f,
			0.f,                0.f,                2.f * f * n / (n - f),  0.f);
	}
};

} // namespace

bool
hook::render_frame(GLuint /* prime_fbo */)
{
	if (!check_context(__FUNCTION__))
		return false;

	const simd::matx4 r0 = matx4_rotate(g_angle - M_PI_2, 1.f, 0.f, 0.f);
	const simd::matx4 r1 = matx4_rotate(g_angle,          0.f, 1.f, 0.f);
	const simd::matx4 r2 = matx4_rotate(g_angle,          0.f, 0.f, 1.f);

	const simd::matx4 p0 = simd::matx4().mul(r0, r1);
	const simd::matx4 p1 = simd::matx4().mul(p0, r2);

	g_angle += g_angle_step;

	const float z_offset = -2.125f;
	const simd::vect4 translate(0.f, 0.f, z_offset, 0.f);

	simd::matx4 mv = simd::matx4().mul(g_matx_fit, p1);
	mv.set(3, simd::vect4().add(mv[3], translate));

	const float l = -.5f;
	const float r = .5f;
	const float b = -.5f;
	const float t = .5f;
	const float n = 1.f;
	const float f = 4.f;

	const matx4_persp proj(l, r, b, t, n, f);
	const simd::matx4 mvp = simd::matx4().mul(mv, proj);

	const simd::vect3 lp_obj = simd::vect3(
		mv[0][0] + mv[0][2],
		mv[1][0] + mv[1][2],
		mv[2][0] + mv[2][2]).normalise();

	GLint vp[4];
	glGetIntegerv(GL_VIEWPORT, vp);
	const float aspect = float(vp[3]) / vp[2];

	const rend::dense_matx4 dense_mvp = rend::dense_matx4(
			mvp[0][0] * aspect, mvp[0][1], mvp[0][2], mvp[0][3],
			mvp[1][0] * aspect, mvp[1][1], mvp[1][2], mvp[1][3],
			mvp[2][0] * aspect, mvp[2][1], mvp[2][2], mvp[2][3],
			mvp[3][0] * aspect, mvp[3][1], mvp[3][2], mvp[3][3]);

	/////////////////////////////////////////////////////////////////

	glEnable(GL_DEPTH_TEST);

	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glUseProgram(g_shader_prog[PROG_SKIN]);

	DEBUG_GL_ERR()

	if (-1 != g_uni[PROG_SKIN][UNI_MVP]) {
		glUniformMatrix4fv(g_uni[PROG_SKIN][UNI_MVP],
			1, GL_FALSE, static_cast< const GLfloat* >(dense_mvp));
	}

	DEBUG_GL_ERR()

	if (-1 != g_uni[PROG_SKIN][UNI_LP_OBJ]) {
		const GLfloat nonlocal_light[4] = {
			lp_obj[0],
			lp_obj[1],
			lp_obj[2],
			0.f
		};

		glUniform4fv(g_uni[PROG_SKIN][UNI_LP_OBJ], 1, nonlocal_light);
	}

	DEBUG_GL_ERR()

	if (-1 != g_uni[PROG_SKIN][UNI_VP_OBJ]) {
		const GLfloat nonlocal_viewer[4] = {
			mv[0][2],
			mv[1][2],
			mv[2][2],
			0.f
		};

		glUniform4fv(g_uni[PROG_SKIN][UNI_VP_OBJ], 1, nonlocal_viewer);
	}

	DEBUG_GL_ERR()

#if PLATFORM_GL_OES_vertex_array_object
	glBindVertexArrayOES(g_vao[PROG_SKIN]);

	DEBUG_GL_ERR()

#else
	if (!bindVertexBuffersAndPointers< PROG_SKIN >())
		return false;

	for (unsigned i = 0; i < g_active_attr_semantics[PROG_SKIN].num_active_attr; ++i)
		glEnableVertexAttribArray(g_active_attr_semantics[PROG_SKIN].active_attr[i]);

	DEBUG_GL_ERR()

#endif
	glDrawElements(GL_TRIANGLES, g_num_faces[MESH_SKIN] * 3, g_index_type, 0);

	DEBUG_GL_ERR()

#if PLATFORM_GL_OES_vertex_array_object == 0
	for (unsigned i = 0; i < g_active_attr_semantics[PROG_SKIN].num_active_attr; ++i)
		glDisableVertexAttribArray(g_active_attr_semantics[PROG_SKIN].active_attr[i]);

	DEBUG_GL_ERR()

#endif
	return true;
}
