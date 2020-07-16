#if PLATFORM_GL
	#include <GL/gl.h>
	#include "gles_gl_mapping.hpp"
#else
	#include <EGL/egl.h>
	#include <GLES2/gl2.h>
	#include <GLES2/gl2ext.h>
	#include "gles_ext.h"
#endif

#include <unistd.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>
#include <math.h>

#include "scoped.hpp"
#include "stream.hpp"
#include "util_tex.hpp"
#include "util_misc.hpp"
#include "pure_macro.hpp"

#include "rendVertAttr.hpp"

using util::scoped_ptr;
using util::scoped_functor;
using util::deinit_resources_t;

namespace { // anonymous

#define SETUP_VERTEX_ATTR_POINTERS_MASK ( \
		SETUP_VERTEX_ATTR_POINTERS_MASK_vert2d)

#include "rendVertAttr_setupVertAttrPointers.hpp"
#undef SETUP_VERTEX_ATTR_POINTERS_MASK

struct Vertex {
	GLfloat pos[2];
};

} // namespace

namespace { // anonymous

const char* arg_prefix    = "-";
const char* arg_app       = "app";

const char* arg_anim_step = "anim_step";

float g_angle = 0.f;
float g_angle_step = .0125f;

#if PLATFORM_EGL
EGLDisplay g_display = EGL_NO_DISPLAY;
EGLContext g_context = EGL_NO_CONTEXT;

#endif
enum {
	PROG_CONIC,

	PROG_COUNT,
	PROG_FORCE_UINT = -1U
};

enum {
	UNI_ASPECT,
	UNI_SEGMENT,

	UNI_COUNT,
	UNI_FORCE_UINT = -1U
};

enum {
	MESH_CONIC,

	MESH_COUNT,
	MESH_FORCE_UINT = -1U
};

enum {
	VBO_CONIC_VTX,
	VBO_CONIC_IDX,

	VBO_COUNT,
	VBO_FORCE_UINT = -1U
};

GLint g_uni[PROG_COUNT][UNI_COUNT];

#if PLATFORM_GL_OES_vertex_array_object
GLuint g_vao[PROG_COUNT];

#endif
GLuint g_vbo[VBO_COUNT];
GLuint g_shader_vert[PROG_COUNT];
GLuint g_shader_frag[PROG_COUNT];
GLuint g_shader_prog[PROG_COUNT];

unsigned g_num_faces[MESH_COUNT];

rend::ActiveAttrSemantics g_active_attr_semantics[PROG_COUNT];

} // namespace anonymous

namespace hook {

bool set_num_drawcalls(
	const unsigned)
{
	return false;
}

unsigned get_num_drawcalls()
{
	return 1;
}

bool requires_depth()
{
	return false;
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
			"\t" << arg_prefix << arg_app << " " << arg_anim_step <<
			" <step>\t: use specified rotation step\n\n";
	}

	return !cli_err;
}

template < typename T >
class generic_free
{
public:
	void operator()(T* arg)
	{
		free(arg);
	}
};

static bool check_context(
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

bool deinit_resources()
{
	if (!check_context(__FUNCTION__))
		return false;

	for (unsigned i = 0; i < sizeof(g_shader_prog) / sizeof(g_shader_prog[0]); ++i)
	{
		glDeleteProgram(g_shader_prog[i]);
		g_shader_prog[i] = 0;
	}

	for (unsigned i = 0; i < sizeof(g_shader_vert) / sizeof(g_shader_vert[0]); ++i)
	{
		glDeleteShader(g_shader_vert[i]);
		g_shader_vert[i] = 0;
	}

	for (unsigned i = 0; i < sizeof(g_shader_frag) / sizeof(g_shader_frag[0]); ++i)
	{
		glDeleteShader(g_shader_frag[i]);
		g_shader_frag[i] = 0;
	}

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

#if DEBUG && PLATFORM_GL_KHR_debug
static void debugProc(
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
bool init_resources(
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
	glDisable(GL_DEPTH_TEST);

	/////////////////////////////////////////////////////////////////

	for (unsigned i = 0; i < PROG_COUNT; ++i)
		for (unsigned j = 0; j < UNI_COUNT; ++j)
			g_uni[i][j] = -1;

	/////////////////////////////////////////////////////////////////

	g_shader_vert[PROG_CONIC] = glCreateShader(GL_VERTEX_SHADER);
	assert(g_shader_vert[PROG_CONIC]);

	if (!util::setupShader(g_shader_vert[PROG_CONIC], "asset/shader/conic.glslv")) {
		stream::cerr << __FUNCTION__ << " failed at setupShader\n";
		return false;
	}

	g_shader_frag[PROG_CONIC] = glCreateShader(GL_FRAGMENT_SHADER);
	assert(g_shader_frag[PROG_CONIC]);

	if (!util::setupShader(g_shader_frag[PROG_CONIC], "asset/shader/conic.glslf")) {
		stream::cerr << __FUNCTION__ << " failed at setupShader\n";
		return false;
	}

	g_shader_prog[PROG_CONIC] = glCreateProgram();
	assert(g_shader_prog[PROG_CONIC]);

	if (!util::setupProgram(
			g_shader_prog[PROG_CONIC],
			g_shader_vert[PROG_CONIC],
			g_shader_frag[PROG_CONIC]))
	{
		stream::cerr << __FUNCTION__ << " failed at setupProgram\n";
		return false;
	}

	g_uni[PROG_CONIC][UNI_ASPECT]  = glGetUniformLocation(g_shader_prog[PROG_CONIC], "aspect");
	g_uni[PROG_CONIC][UNI_SEGMENT] = glGetUniformLocation(g_shader_prog[PROG_CONIC], "segment");

	g_active_attr_semantics[PROG_CONIC].registerVertexAttr(glGetAttribLocation(g_shader_prog[PROG_CONIC], "at_position"));

	/////////////////////////////////////////////////////////////////

#if PLATFORM_GL_OES_vertex_array_object
	glGenVertexArraysOES(sizeof(g_vao) / sizeof(g_vao[0]), g_vao);

	for (unsigned i = 0; i < sizeof(g_vao) / sizeof(g_vao[0]); ++i)
		assert(g_vao[i]);

#endif
	glGenBuffers(sizeof(g_vbo) / sizeof(g_vbo[0]), g_vbo);

	for (unsigned i = 0; i < sizeof(g_vbo) / sizeof(g_vbo[0]); ++i)
		assert(g_vbo[i]);

	static const GLfloat vertex_buffer_data[] = { 
		-1.0f, -1.0f,
		 1.0f, -1.0f,
		-1.0f,	1.0f,
		 1.0f,	1.0f
	};
	static const GLushort element_buffer_data[] = { 0, 1, 2, 3 };

	glBindBuffer(GL_ARRAY_BUFFER, g_vbo[VBO_CONIC_VTX]);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertex_buffer_data), vertex_buffer_data, GL_STATIC_DRAW);
	glBindBuffer(GL_ARRAY_BUFFER, 0);

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, g_vbo[VBO_CONIC_IDX]);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(element_buffer_data), element_buffer_data, GL_STATIC_DRAW);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

	g_num_faces[MESH_CONIC] = sizeof(element_buffer_data) / sizeof(element_buffer_data[0]) - 3 + 1;

	if (util::reportGLError()) {
		stream::cerr << __FUNCTION__ << " failed at glBindBuffer/glBufferData for ELEMENT_ARRAY_BUFFER\n";
		return false;
	}

#if PLATFORM_GL_OES_vertex_array_object
	glBindVertexArrayOES(g_vao[PROG_CONIC]);

#endif
	glBindBuffer(GL_ARRAY_BUFFER, g_vbo[VBO_CONIC_VTX]);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, g_vbo[VBO_CONIC_IDX]);

	if (!setupVertexAttrPointers< Vertex >(g_active_attr_semantics[PROG_CONIC])) {
		stream::cerr << __FUNCTION__ << " failed at setupVertexAttrPointers\n";
		return false;
	}

	on_error.reset();
	return true;
}

class matx3;
static matx3 matx3_mul(const matx3&, const matx3&);

class matx3 {
	friend matx3 matx3_mul(const matx3&, const matx3&);
	float m[3][3];

public:
	matx3() {}

	matx3(
		const float c0_0, const float c0_1, const float c0_2,
		const float c1_0, const float c1_1, const float c1_2,
		const float c2_0, const float c2_1, const float c2_2)
	{
		m[0][0] = c0_0;
		m[0][1] = c0_1;
		m[0][2] = c0_2;
		m[1][0] = c1_0;
		m[1][1] = c1_1;
		m[1][2] = c1_2;
		m[2][0] = c2_0;
		m[2][1] = c2_1;
		m[2][2] = c2_2;
	}

	const float (& operator[](const size_t i) const)[3] { return m[i]; }

	template < size_t NUM_VECT, size_t NUM_ELEM >
	void transform(
		const float (& a)[NUM_VECT][NUM_ELEM],
		float (& out)[NUM_VECT][NUM_ELEM]) const {

		const size_t num_vect = NUM_VECT;
		const size_t num_elem = 3;
		const util::compile_assert< num_elem <= NUM_ELEM > assert_num_elem;

		for (size_t j = 0; j < num_vect; ++j) {
			float out_j[num_elem];
			const float a_j0 = a[j][0];
			for (size_t i = 0; i < num_elem; ++i) {
				out_j[i] = a_j0 * m[0][i];
			}
			for (size_t k = 1; k < num_elem; ++k) {
				const float a_jk = a[j][k];
				for (size_t i = 0; i < num_elem; ++i) {
					out_j[i] += a_jk * m[k][i];
				}
			}
			for (size_t i = 0; i < num_elem; ++i)
				out[j][i] = out_j[i];
			for (size_t i = num_elem; i < NUM_ELEM; ++i)
				out[j][i] = a[j][i];
		}
	}

	template < size_t NUM_VECT, size_t NUM_ELEM >
	void transform(
		const float (&translate)[3],
		const float (& a)[NUM_VECT][NUM_ELEM],
		float (& out)[NUM_VECT][NUM_ELEM]) const {

		const size_t num_vect = NUM_VECT;
		const size_t num_elem = 3;
		const util::compile_assert< num_elem <= NUM_ELEM > assert_num_elem;

		for (size_t j = 0; j < num_vect; ++j) {
			float out_j[num_elem];
			const float a_j0 = a[j][0];
			for (size_t i = 0; i < num_elem; ++i) {
				out_j[i] = a_j0 * m[0][i];
			}
			for (size_t k = 1; k < num_elem; ++k) {
				const float a_jk = a[j][k];
				for (size_t i = 0; i < num_elem; ++i) {
					out_j[i] += a_jk * m[k][i];
				}
			}
			for (size_t i = 0; i < num_elem; ++i)
				out[j][i] = out_j[i] + translate[i];
			for (size_t i = num_elem; i < NUM_ELEM; ++i)
				out[j][i] = a[j][i];
		}
	}
};

static matx3 matx3_mul(
	const matx3& ma,
	const matx3& mb)
{
	matx3 mc;
	mb.transform(ma.m, mc.m);
	return mc;
}

static matx3 matx3_rotate(
	const float a,
	const float x,
	const float y,
	const float z)
{
	const float sin_a = sinf(a);
	const float cos_a = cosf(a);

	return matx3(
		x * x + cos_a * (1 - x * x),         x * y - cos_a * (x * y) + sin_a * z, x * z - cos_a * (x * z) - sin_a * y,
		y * x - cos_a * (y * x) - sin_a * z, y * y + cos_a * (1 - y * y),         y * z - cos_a * (y * z) + sin_a * x,
		z * x - cos_a * (z * x) + sin_a * y, z * y - cos_a * (z * y) - sin_a * x, z * z + cos_a * (1 - z * z));
}

bool render_frame(GLuint /* primary_fbo */)
{
	if (!check_context(__FUNCTION__))
		return false;

	/////////////////////////////////////////////////////////////////

	GLint vp[4];
	glGetIntegerv(GL_VIEWPORT, vp);
	const float aspect = float(vp[3]) / vp[2];

	const matx3 s0 = matx3(
		.5f, 0, 0,
		0, .5f, 0,
		0, 0, .5f);

	const matx3 r0 = matx3_rotate(-M_PI_4, 0.f, 1.f, 0.f);
	const matx3 r1 = matx3_rotate(g_angle, 1.f, 0.f, 0.f);
	const matx3 r2 = matx3_rotate(g_angle, 0.f, 0.f, 1.f);

	const matx3 ps = matx3_mul(s0, r0);
	const matx3 p0 = matx3_mul(ps, r1);
	const matx3 p1 = matx3_mul(p0, r2);

	g_angle = fmodf(g_angle + g_angle_step, 2.f * M_PI);

	/////////////////////////////////////////////////////////////////

	const float segment[][4] = { // end-coords (index 0..2) and radii (index 3)
		{ -.625,  -1.0,   1.0, .125 }, // 0
		{  .625,  -1.0,   1.0, .125 }, // 1
		{  .625,  .625,   1.0, .125 }, // 2
		{  -1.0,  .625,   1.0, .125 }, // 3
		{  -1.0, -.625,   1.0, .125 }, // 4
		{  -1.0, -.625, -.625, .125 }, // 5
		{  -1.0,   1.0, -.625, .125 }, // 6
		{  -1.0,   1.0,  .625, .125 }, // 7
		{  .625,   1.0,  .625, .125 }, // 8
		{  .625,   1.0,  -1.0, .125 }, // 9
		{ -.625,   1.0,  -1.0, .125 }, // a
		{ -.625, -.625,  -1.0, .125 }, // b
		{   1.0, -.625,  -1.0, .125 }, // c
		{   1.0,  .625,  -1.0, .125 }, // d
		{   1.0,  .625,  .625, .125 }, // e
		{   1.0,  -1.0,  .625, .125 }, // f
		{   1.0,  -1.0, -.625, .125 }, // g
		{ -.625,  -1.0, -.625, .125 }, // h
		{ -.625,  -1.0,   1.0, .125 }  // 0 -- wrap-around to avoid expensive modulo ops in the shader
	};

	const float translate[] = { 0, 0, -.125f };
	float segTransformed[sizeof(segment) / sizeof(segment[0])][4];
	p1.transform(translate, segment, segTransformed);

	/////////////////////////////////////////////////////////////////

	glUseProgram(g_shader_prog[PROG_CONIC]);

	DEBUG_GL_ERR()

	if (-1 != g_uni[PROG_CONIC][UNI_ASPECT]) {
		glUniform1f(g_uni[PROG_CONIC][UNI_ASPECT], GLfloat(aspect));

		DEBUG_GL_ERR()
	}

	if (-1 != g_uni[PROG_CONIC][UNI_SEGMENT]) {
		glUniform4fv(g_uni[PROG_CONIC][UNI_SEGMENT], sizeof(segTransformed) / sizeof(segTransformed[0]), segTransformed[0]);

		DEBUG_GL_ERR()
	}

	for (unsigned i = 0; i < g_active_attr_semantics[PROG_CONIC].num_active_attr; ++i)
		glEnableVertexAttribArray(g_active_attr_semantics[PROG_CONIC].active_attr[i]);

	DEBUG_GL_ERR()

	glDrawElements(GL_TRIANGLE_STRIP, g_num_faces[MESH_CONIC] - 1 + 3, GL_UNSIGNED_SHORT, (void*) 0);

	DEBUG_GL_ERR()

	for (unsigned i = 0; i < g_active_attr_semantics[PROG_CONIC].num_active_attr; ++i)
		glDisableVertexAttribArray(g_active_attr_semantics[PROG_CONIC].active_attr[i]);

	DEBUG_GL_ERR()

	return true;
}

} // namespace hook
