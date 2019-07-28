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
#include "util_tex.hpp"
#include "util_misc.hpp"
#include "pure_macro.hpp"
#include "stream.hpp"

#include "rendVertAttr.hpp"

using util::scoped_ptr;
using util::scoped_functor;
using util::deinit_resources_t;

namespace { // anonymous

#define SETUP_VERTEX_ATTR_POINTERS_MASK ( \
		SETUP_VERTEX_ATTR_POINTERS_MASK_vertex  | \
		SETUP_VERTEX_ATTR_POINTERS_MASK_normal  | \
		SETUP_VERTEX_ATTR_POINTERS_MASK_tangent | \
		SETUP_VERTEX_ATTR_POINTERS_MASK_tcoord)

#include "rendVertAttr_setupVertAttrPointers.hpp"
#undef SETUP_VERTEX_ATTR_POINTERS_MASK

struct Vertex {
	GLfloat pos[3];
	GLfloat nrm[3];
	GLfloat tan[3];
	GLfloat txc[2];
};

const char arg_prefix[]    = "-";
const char arg_app[]       = "app";

const char arg_normal[]    = "normal_map";
const char arg_albedo[]    = "albedo_map";
const char arg_tile[]      = "tile";
const char arg_anim_step[] = "anim_step";

struct TexDesc {
	const char* filename;
	unsigned w;
	unsigned h;
};

TexDesc g_normal = { "asset/texture/slate_normal.raw", 1024, 1024 };
TexDesc g_albedo = { "asset/texture/slate_albedo.raw", 1024, 1024 };

float g_tile = 2.f;
float g_angle = 0.f;
float g_angle_step = .0125f;

#if PLATFORM_EGL
EGLDisplay g_display = EGL_NO_DISPLAY;
EGLContext g_context = EGL_NO_CONTEXT;

#endif
enum {
	TEX_NORMAL,
	TEX_ALBEDO,

	TEX_COUNT,
	TEX_FORCE_UINT = -1U
};

enum {
	PROG_SPHERE,

	PROG_COUNT,
	PROG_FORCE_UINT = -1U
};

enum {
	UNI_SAMPLER_NORMAL,
	UNI_SAMPLER_ALBEDO,

	UNI_LP_OBJ,
	UNI_VP_OBJ,
	UNI_MVP,

	UNI_COUNT,
	UNI_FORCE_UINT = -1U
};

enum {
	MESH_SPHERE,

	MESH_COUNT,
	MESH_FORCE_UINT = -1U
};

enum {
	VBO_SPHERE_VTX,
	VBO_SPHERE_IDX,

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

rend::ActiveAttrSemantics g_active_attr_semantics[PROG_COUNT];

} // namespace

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

} // namespace

namespace { // anonymous

bool parse_cli(
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
			if (i + 3 < argc && !strcmp(argv[i], arg_normal)) {
				if (1 == sscanf(argv[i + 2], "%u", &g_normal.w) &&
					1 == sscanf(argv[i + 3], "%u", &g_normal.h)) {

					g_normal.filename = argv[i + 1];
					i += 3;
					continue;
				}
			}
			else
			if (i + 3 < argc && !strcmp(argv[i], arg_albedo)) {
				if (1 == sscanf(argv[i + 2], "%u", &g_albedo.w) &&
					1 == sscanf(argv[i + 3], "%u", &g_albedo.h)) {

					g_albedo.filename = argv[i + 1];
					i += 3;
					continue;
				}
			}
			else
			if (i + 1 < argc && !strcmp(argv[i], arg_tile)) {
				if (1 == sscanf(argv[i + 1], "%f", &g_tile) && 0.f < g_tile) {
					i += 1;
					continue;
				}
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
			"\t" << arg_prefix << arg_app << " " << arg_normal <<
			" <filename> <width> <height>\t: use specified raw file and dimensions as source of normal map\n"
			"\t" << arg_prefix << arg_app << " " << arg_albedo <<
			" <filename> <width> <height>\t: use specified raw file and dimensions as source of albedo map\n"
			"\t" << arg_prefix << arg_app << " " << arg_tile <<
			" <n>\t\t\t\t\t: tile texture maps the specified number of times along U, half as much along V\n"
			"\t" << arg_prefix << arg_app << " " << arg_anim_step <<
			" <step>\t\t\t\t: use specified rotation step\n\n";
	}

	return cli_err;
}

template < typename T >
class generic_free {
public:
	void operator()(T* arg) {
		free(arg);
	}
};

// bend a polar sphere from a grid of the specified dimensions
bool createIndexedPolarSphere(
	const GLuint vbo_arr,
	const GLuint vbo_idx,
	unsigned& num_faces,
	const int aspect_u_over_v,
	const int rows = 33,
	const int cols = 65)
{
	assert(vbo_arr && vbo_idx);

	const float r = 1.f;

	assert(rows > 2);
	assert(cols > 3);

	typedef uint16_t Index;
	const size_t num_verts = (rows - 2) * cols + 2 * (cols - 1);
	const size_t num_tris = ((rows - 3) * 2 + 2) * (cols - 1);
	num_faces = num_tris;

	scoped_ptr< Vertex, generic_free > arr(
		reinterpret_cast< Vertex* >(malloc(sizeof(Vertex) * num_verts)));
	unsigned ai = 0;

	// north pole
	for (int j = 0; j < cols - 1; ++j) {
		assert(ai < num_verts);

		const float azim = j * 2 * M_PI / (cols - 1);
		const float sin_azim = sinf(azim);
		const float cos_azim = cosf(azim);

		arr()[ai].pos[0] = 0.f;
		arr()[ai].pos[1] = 0.f;
		arr()[ai].pos[2] = r;

		arr()[ai].nrm[0] = 0.f;
		arr()[ai].nrm[1] = 0.f;
		arr()[ai].nrm[2] = 1.f;

		arr()[ai].tan[0] = -sin_azim;
		arr()[ai].tan[1] = cos_azim;
		arr()[ai].tan[2] = 0;

		arr()[ai].txc[0] = g_tile * (j + .5f) / (cols - 1);
		arr()[ai].txc[1] = g_tile * (1.f / aspect_u_over_v);
		++ai;
	}

	// interior
	for (int i = 1; i < rows - 1; ++i)
		for (int j = 0; j < cols; ++j) {
			assert(ai < num_verts);

			const float azim = j * 2 * M_PI / (cols - 1);
			const float decl = M_PI_2 - i * M_PI / (rows - 1);
			const float sin_azim = sinf(azim);
			const float cos_azim = cosf(azim);
			const float sin_decl = sinf(decl);
			const float cos_decl = cosf(decl);

			arr()[ai].pos[0] = r * cos_decl * cos_azim;
			arr()[ai].pos[1] = r * cos_decl * sin_azim;
			arr()[ai].pos[2] = r * sin_decl;

			arr()[ai].nrm[0] = cos_decl * cos_azim;
			arr()[ai].nrm[1] = cos_decl * sin_azim;
			arr()[ai].nrm[2] = sin_decl;

			arr()[ai].tan[0] = -sin_azim;
			arr()[ai].tan[1] = cos_azim;
			arr()[ai].tan[2] = 0;

			arr()[ai].txc[0] = g_tile * j / (cols - 1);
			arr()[ai].txc[1] = g_tile * (rows - 1 - i) / (aspect_u_over_v * rows - aspect_u_over_v);
			++ai;
		}

	// south pole
	for (int j = 0; j < cols - 1; ++j) {
		assert(ai < num_verts);

		const float azim = j * 2 * M_PI / (cols - 1);
		const float sin_azim = sinf(azim);
		const float cos_azim = cosf(azim);

		arr()[ai].pos[0] = 0.f;
		arr()[ai].pos[1] = 0.f;
		arr()[ai].pos[2] = -r;

		arr()[ai].nrm[0] = 0.f;
		arr()[ai].nrm[1] = 0.f;
		arr()[ai].nrm[2] = -1.f;

		arr()[ai].tan[0] = -sin_azim;
		arr()[ai].tan[1] = cos_azim;
		arr()[ai].tan[2] = 0;

		arr()[ai].txc[0] = g_tile * (j + .5f) / (cols - 1);
		arr()[ai].txc[1] = 0.f;
		++ai;
	}

	assert(ai == num_verts);

	scoped_ptr< Index[3], generic_free > idx(
		reinterpret_cast< Index(*)[3] >(malloc(sizeof(Index[3]) * num_tris)));
	unsigned ii = 0;

	// north pole
	for (unsigned j = 0; j < cols - 1; ++j) {
		assert(ii < num_tris);
		idx()[ii][0] = Index(j);
		idx()[ii][1] = Index(j + cols - 1);
		idx()[ii][2] = Index(j + cols);
		++ii;
	}

	// interior
	for (int i = 1; i < rows - 2; ++i)
		for (int j = 0; j < cols - 1; ++j) {
			assert(ii < num_tris);
			idx()[ii][0] = Index(j + i * cols);
			idx()[ii][1] = Index(j + i * cols - 1);
			idx()[ii][2] = Index(j + (i + 1) * cols);
			++ii;

			assert(ii < num_tris);
			idx()[ii][0] = Index(j + (i + 1) * cols - 1);
			idx()[ii][1] = Index(j + (i + 1) * cols);
			idx()[ii][2] = Index(j + i * cols - 1);
			++ii;
		}

	// south pole
	for (unsigned j = 0; j < cols - 1; ++j) {
		assert(ii < num_tris);
		idx()[ii][0] = Index(j + (rows - 2) * cols);
		idx()[ii][1] = Index(j + (rows - 2) * cols - 1);
		idx()[ii][2] = Index(j + (rows - 2) * cols + cols - 1);
		++ii;
	}

	assert(ii == num_tris);
	stream::cout << "number of vertices: " << num_verts << "\nnumber of faces: " << num_tris << '\n';

	glBindBuffer(GL_ARRAY_BUFFER, vbo_arr);
	glBufferData(GL_ARRAY_BUFFER, sizeof(*arr()) * num_verts, arr(), GL_STATIC_DRAW);
	glBindBuffer(GL_ARRAY_BUFFER, 0);

	if (util::reportGLError()) {
		stream::cerr << __FUNCTION__ <<
			" failed at glBindBuffer/glBufferData for ARRAY_BUFFER\n";
		return false;
	}

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, vbo_idx);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(*idx()) * num_tris, idx(), GL_STATIC_DRAW);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

	if (util::reportGLError()) {
		stream::cerr << __FUNCTION__ <<
			" failed at glBindBuffer/glBufferData for ELEMENT_ARRAY_BUFFER\n";
		return false;
	}

	return true;
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

namespace hook {

bool deinit_resources()
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

} // namespace

namespace hook {

bool init_resources(
	const unsigned argc,
	const char* const * argv)
{
	if (parse_cli(argc, argv))
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
	// set up misc control bits and values

	glEnable(GL_CULL_FACE);
	glDisable(GL_DEPTH_TEST);

	const GLclampf red = 0.f;
	const GLclampf green = 0.f;
	const GLclampf blue = 0.f;
	const GLclampf alpha = 0.f;

	glClearColor(red, green, blue, alpha);

	/////////////////////////////////////////////////////////////////
	// reserve all necessary texture objects

	glGenTextures(sizeof(g_tex) / sizeof(g_tex[0]), g_tex);

	for (unsigned i = 0; i < sizeof(g_tex) / sizeof(g_tex[0]); ++i)
		assert(g_tex[i]);

	/////////////////////////////////////////////////////////////////
	// load textures

	if (!util::setupTexture2D(g_tex[TEX_NORMAL], g_normal.filename, g_normal.w, g_normal.h)) {
		stream::cerr << __FUNCTION__ << " failed at setupTexture2D\n";
		return false;
	}

	if (!util::setupTexture2D(g_tex[TEX_ALBEDO], g_albedo.filename, g_albedo.w, g_albedo.h)) {
		stream::cerr << __FUNCTION__ << " failed at setupTexture2D\n";
		return false;
	}

	/////////////////////////////////////////////////////////////////
	// init the program/uniforms matrix to all empty

	for (unsigned i = 0; i < PROG_COUNT; ++i)
		for (unsigned j = 0; j < UNI_COUNT; ++j)
			g_uni[i][j] = -1;

	/////////////////////////////////////////////////////////////////
	// create the shader program from two shaders

	g_shader_vert[PROG_SPHERE] = glCreateShader(GL_VERTEX_SHADER);
	assert(g_shader_vert[PROG_SPHERE]);

	if (!util::setupShader(g_shader_vert[PROG_SPHERE], "asset/shader/blinn_bump_tang.glslv")) {
		stream::cerr << __FUNCTION__ << " failed at setupShader\n";
		return false;
	}

	g_shader_frag[PROG_SPHERE] = glCreateShader(GL_FRAGMENT_SHADER);
	assert(g_shader_frag[PROG_SPHERE]);

	if (!util::setupShader(g_shader_frag[PROG_SPHERE], "asset/shader/blinn_bump_tang.glslf")) {
		stream::cerr << __FUNCTION__ << " failed at setupShader\n";
		return false;
	}

	g_shader_prog[PROG_SPHERE] = glCreateProgram();
	assert(g_shader_prog[PROG_SPHERE]);

	if (!util::setupProgram(
			g_shader_prog[PROG_SPHERE],
			g_shader_vert[PROG_SPHERE],
			g_shader_frag[PROG_SPHERE]))
	{
		stream::cerr << __FUNCTION__ << " failed at setupProgram\n";
		return false;
	}

	/////////////////////////////////////////////////////////////////
	// query the program about known uniform vars and vertex attribs

	g_uni[PROG_SPHERE][UNI_MVP]    = glGetUniformLocation(g_shader_prog[PROG_SPHERE], "mvp");
	g_uni[PROG_SPHERE][UNI_LP_OBJ] = glGetUniformLocation(g_shader_prog[PROG_SPHERE], "lp_obj");
	g_uni[PROG_SPHERE][UNI_VP_OBJ] = glGetUniformLocation(g_shader_prog[PROG_SPHERE], "vp_obj");

	g_uni[PROG_SPHERE][UNI_SAMPLER_NORMAL] = glGetUniformLocation(g_shader_prog[PROG_SPHERE], "normal_map");
	g_uni[PROG_SPHERE][UNI_SAMPLER_ALBEDO] = glGetUniformLocation(g_shader_prog[PROG_SPHERE], "albedo_map");

	g_active_attr_semantics[PROG_SPHERE].registerVertexAttr(glGetAttribLocation(g_shader_prog[PROG_SPHERE], "at_Vertex"));
	g_active_attr_semantics[PROG_SPHERE].registerNormalAttr(glGetAttribLocation(g_shader_prog[PROG_SPHERE], "at_Normal"));
	g_active_attr_semantics[PROG_SPHERE].registerTangentAttr(glGetAttribLocation(g_shader_prog[PROG_SPHERE], "at_Tangent"));
	g_active_attr_semantics[PROG_SPHERE].registerTCoordAttr(glGetAttribLocation(g_shader_prog[PROG_SPHERE], "at_MultiTexCoord0"));

	/////////////////////////////////////////////////////////////////
	// reserve VAO (if available) and all necessary VBOs

#if PLATFORM_GL_OES_vertex_array_object
	glGenVertexArraysOES(sizeof(g_vao) / sizeof(g_vao[0]), g_vao);

	for (unsigned i = 0; i < sizeof(g_vao) / sizeof(g_vao[0]); ++i)
		assert(g_vao[i]);

	glBindVertexArrayOES(g_vao[PROG_SPHERE]);

#endif
	glGenBuffers(sizeof(g_vbo) / sizeof(g_vbo[0]), g_vbo);

	for (unsigned i = 0; i < sizeof(g_vbo) / sizeof(g_vbo[0]); ++i)
		assert(g_vbo[i]);

	/////////////////////////////////////////////////////////////////
	// set up the vertex attrib mapping for all attrib inputs

	glBindBuffer(GL_ARRAY_BUFFER, g_vbo[VBO_SPHERE_VTX]);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, g_vbo[VBO_SPHERE_IDX]);

	if (!setupVertexAttrPointers< Vertex >(g_active_attr_semantics[PROG_SPHERE])) {
		stream::cerr << __FUNCTION__ << " failed at setupVertexAttrPointers\n";
		return false;
	}

#if PLATFORM_GL_OES_vertex_array_object
	/////////////////////////////////////////////////////////////////
	// if VAO: the final step of enabling the mapped attrib inputs

	for (unsigned i = 0; i < g_active_attr_semantics[PROG_SPHERE].num_active_attr; ++i)
		glEnableVertexAttribArray(g_active_attr_semantics[PROG_SPHERE].active_attr[i]);

	DEBUG_GL_ERR()

	glBindVertexArrayOES(0);

#endif
	/////////////////////////////////////////////////////////////////
	// unbind the VBOs -- will be re-bound on a need-to-use basis

	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

	/////////////////////////////////////////////////////////////////
	// produce some geometrical asset and put that in the VBOs

	if (!createIndexedPolarSphere(
			g_vbo[VBO_SPHERE_VTX],
			g_vbo[VBO_SPHERE_IDX],
			g_num_faces[MESH_SPHERE],
			g_normal.w == g_normal.h ? 2 : 1))
	{
		stream::cerr << __FUNCTION__ << " failed at createIndexedPolarSphere\n";
		return false;
	}

	on_error.reset();
	return true;
}

} // namespace

namespace { // anonymous

class matx3;
matx3 matx3_mul(const matx3&, const matx3&);

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
};

matx3 matx3_mul(
	const matx3& ma,
	const matx3& mb)
{
	matx3 mc;
	mb.transform(ma.m, mc.m);
	return mc;
}

matx3 matx3_rotate(
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

} // namespace

namespace hook {

bool render_frame(GLuint /* prime_fbo */)
{
	if (!check_context(__FUNCTION__))
		return false;

	/////////////////////////////////////////////////////////////////
	// clear the framebuffer (color-only)

	glClear(GL_COLOR_BUFFER_BIT);

	/////////////////////////////////////////////////////////////////
	// query about the viewport geometry

	GLint vp[4];
	glGetIntegerv(GL_VIEWPORT, vp);

	/////////////////////////////////////////////////////////////////
	// produce rotational matrix from Euler angles

	const matx3 r0 = matx3_rotate(g_angle - M_PI_2, 1.f, 0.f, 0.f);
	const matx3 r1 = matx3_rotate(g_angle,          0.f, 1.f, 0.f);
	const matx3 r2 = matx3_rotate(g_angle,          0.f, 0.f, 1.f);

	const matx3 p0 = matx3_mul(r0, r1);
	const matx3 p1 = matx3_mul(p0, r2);

	const float aspect = float(vp[3]) / vp[2];

	// expand to 4x4, sign-inverting z in all original columns (for GL screen space)
	const GLfloat mvp[4][4] = {
		{  p1[0][0] * aspect,  p1[0][1], -p1[0][2],  0.f },
		{  p1[1][0] * aspect,  p1[1][1], -p1[1][2],  0.f },
		{  p1[2][0] * aspect,  p1[2][1], -p1[2][2],  0.f },
		{  0.f,                0.f,       0.f,       1.f }
	};

	g_angle = fmodf(g_angle + g_angle_step, 2.f * M_PI);

	/////////////////////////////////////////////////////////////////
	// activate the shader program and set up all valid uniform vars

	glUseProgram(g_shader_prog[PROG_SPHERE]);

	DEBUG_GL_ERR()

	if (-1 != g_uni[PROG_SPHERE][UNI_MVP]) {
		glUniformMatrix4fv(g_uni[PROG_SPHERE][UNI_MVP],
			1, GL_FALSE, reinterpret_cast< const GLfloat* >(mvp));
	}

	DEBUG_GL_ERR()

	if (-1 != g_uni[PROG_SPHERE][UNI_LP_OBJ]) {
		const GLfloat nonlocal_light[4] = {
			p1[0][2],
			p1[1][2],
			p1[2][2],
			0.f
		};

		glUniform4fv(g_uni[PROG_SPHERE][UNI_LP_OBJ], 1, nonlocal_light);
	}

	DEBUG_GL_ERR()

	if (-1 != g_uni[PROG_SPHERE][UNI_VP_OBJ]) {
		const GLfloat nonlocal_viewer[4] = {
			p1[0][2],
			p1[1][2],
			p1[2][2],
			0.f
		};

		glUniform4fv(g_uni[PROG_SPHERE][UNI_VP_OBJ], 1, nonlocal_viewer);
	}

	DEBUG_GL_ERR()

	if (0 != g_tex[TEX_NORMAL] && -1 != g_uni[PROG_SPHERE][UNI_SAMPLER_NORMAL]) {
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, g_tex[TEX_NORMAL]);

		glUniform1i(g_uni[PROG_SPHERE][UNI_SAMPLER_NORMAL], 0);
	}

	DEBUG_GL_ERR()

	if (0 != g_tex[TEX_ALBEDO] && -1 != g_uni[PROG_SPHERE][UNI_SAMPLER_ALBEDO]) {
		glActiveTexture(GL_TEXTURE1);
		glBindTexture(GL_TEXTURE_2D, g_tex[TEX_ALBEDO]);

		glUniform1i(g_uni[PROG_SPHERE][UNI_SAMPLER_ALBEDO], 1);
	}

	DEBUG_GL_ERR()

#if PLATFORM_GL_OES_vertex_array_object
	glBindVertexArrayOES(g_vao[PROG_SPHERE]);

#else
	/////////////////////////////////////////////////////////////////
	// no VAO: re-bind the VBOs and enable all mapped vertex attribs

	glBindBuffer(GL_ARRAY_BUFFER, g_vbo[VBO_SPHERE_VTX]);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, g_vbo[VBO_SPHERE_IDX]);

	for (unsigned i = 0; i < g_active_attr_semantics[PROG_SPHERE].num_active_attr; ++i)
		glEnableVertexAttribArray(g_active_attr_semantics[PROG_SPHERE].active_attr[i]);

	DEBUG_GL_ERR()

#endif
	glDrawElements(GL_TRIANGLES, g_num_faces[MESH_SPHERE] * 3, GL_UNSIGNED_SHORT, (void*) 0);

	DEBUG_GL_ERR()

#if PLATFORM_GL_OES_vertex_array_object == 0
	/////////////////////////////////////////////////////////////////
	// no VAO: disable all mapped vertex attribs and unbind VBOs

	for (unsigned i = 0; i < g_active_attr_semantics[PROG_SPHERE].num_active_attr; ++i)
		glDisableVertexAttribArray(g_active_attr_semantics[PROG_SPHERE].active_attr[i]);

	DEBUG_GL_ERR()

	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

#endif
	return true;
}

} // namespace hook
