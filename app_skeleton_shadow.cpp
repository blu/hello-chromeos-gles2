#if PLATFORM_GL
	#include <GL/gl.h>
	#include <GL/glext.h>
	#include "gles_gl_mapping.hpp"
#else
	#include <EGL/egl.h>
	#include <GLES3/gl3.h>
	#include "gles_ext.h"

	#if GL_OES_depth_texture == 0
	#error Missing required extension GL_OES_depth_texture.
	#endif

	#if GL_OES_depth24 == 0
	#error Missing required extension GL_OES_depth24.
	#endif

	#if GL_OES_packed_depth_stencil == 0
	#error Missing required extension GL_OES_packed_depth_stencil.
	#endif

#endif // PLATFORM_GL
#include <unistd.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>
#include <cmath>
#include <string>
#include <sstream>

#include "scoped.hpp"
#include "stream.hpp"
#include "vectsimd.hpp"
#include "util_tex.hpp"
#include "util_misc.hpp"
#include "pure_macro.hpp"

#include "rendIndexedTrilist.hpp"
#include "rendSkeleton.hpp"
#include "rendVertAttr.hpp"

using util::scoped_ptr;
using util::scoped_functor;
using util::deinit_resources_t;

namespace sk {

#define SETUP_VERTEX_ATTR_POINTERS_MASK	( \
		SETUP_VERTEX_ATTR_POINTERS_MASK_vertex | \
		SETUP_VERTEX_ATTR_POINTERS_MASK_normal | \
		SETUP_VERTEX_ATTR_POINTERS_MASK_blendw | \
		SETUP_VERTEX_ATTR_POINTERS_MASK_tcoord)

#include "rendVertAttr_setupVertAttrPointers.hpp"
#undef SETUP_VERTEX_ATTR_POINTERS_MASK

struct Vertex {
	GLfloat pos[3];
	GLfloat	bon[4];
	GLfloat nrm[3];
	GLfloat txc[2];
};

} // namespace sk

namespace st {

#define SETUP_VERTEX_ATTR_POINTERS_MASK	( \
		SETUP_VERTEX_ATTR_POINTERS_MASK_vertex)

#include "rendVertAttr_setupVertAttrPointers.hpp"
#undef SETUP_VERTEX_ATTR_POINTERS_MASK

struct Vertex {
	GLfloat pos[3];
};

} // namespace st

#define DRAW_SKELETON			0
#define DEPTH_PRECISION_24		0

namespace { // anonymous

const char arg_prefix[]     = "--";
const char arg_app[]        = "app";

const char arg_normal[]     = "normal_map";
const char arg_albedo[]     = "albedo_map";
const char arg_anim_step[]  = "anim_step";
const char arg_shadow_res[] = "shadow_res";

struct TexDesc {
	const char* filename;
	unsigned w;
	unsigned h;
};

TexDesc g_normal = { "asset/texture/unperturbed_normal.raw", 8, 8 };
TexDesc g_albedo = { "none", 16, 16 };

float g_anim_step = .0125f;
simd::matx4 g_matx_fit;

const unsigned fbo_default_res = 2048;
GLsizei g_fbo_res = fbo_default_res;

enum {
	BONE_CAPACITY = 32
};

unsigned g_bone_count;
rend::Bone g_bone[BONE_CAPACITY + 1];
rend::Bone* g_root_bone = g_bone + BONE_CAPACITY;
rend::dense_matx4 g_bone_mat[BONE_CAPACITY];
std::vector< std::vector< rend::Track > > g_animations;
std::vector< float > g_durations;

} // namespace

namespace anim {

static std::vector< std::vector< rend::Track > >::const_iterator at;
static std::vector< float >::const_iterator dt;
static float animTime;
static float duration;

} // namespace anim

namespace { // anonymous

#if DRAW_SKELETON
st::Vertex g_stick[BONE_CAPACITY][2];

#endif
#if PLATFORM_EGL
EGLDisplay g_display = EGL_NO_DISPLAY;
EGLContext g_context = EGL_NO_CONTEXT;

#endif
enum {
	TEX_NORMAL,
	TEX_ALBEDO,
	TEX_SHADOW,

	TEX_COUNT,
	TEX_FORCE_UINT = -1U
};

enum {
	PROG_SKIN,
	PROG_SKEL,
	PROG_SHADOW,

	PROG_COUNT,
	PROG_FORCE_UINT = -1U
};

enum {
	UNI_SAMPLER_NORMAL,
	UNI_SAMPLER_ALBEDO,
	UNI_SAMPLER_SHADOW,

	UNI_LP_OBJ,
	UNI_VP_OBJ,
	UNI_SOLID_COLOR,

	UNI_BONE,
	UNI_MVP,
	UNI_MVP_LIT,

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
	VBO_SKEL_VTX,
	/* VBO_SKEL_IDX not required */

	VBO_COUNT,
	VBO_FORCE_UINT = -1U
};

GLint g_uni[PROG_COUNT][UNI_COUNT];

#if PLATFORM_GL_OES_vertex_array_object
GLuint g_vao[PROG_COUNT];

#endif
GLuint g_fbo; // single
GLuint g_tex[TEX_COUNT];
GLuint g_vbo[VBO_COUNT];
GLuint g_shader_vert[PROG_COUNT];
GLuint g_shader_frag[PROG_COUNT];
GLuint g_shader_prog[PROG_COUNT];

unsigned g_num_faces[MESH_COUNT];
GLenum g_index_type;

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
	return true;
}

int parse_cli(
	const unsigned argc,
	const char* const* argv)
{
	unsigned i = 0;

	if (i + 3 < argc && !strcmp(argv[i], arg_normal)) {
		if (1 == sscanf(argv[i + 2], "%u", &g_normal.w) &&
			1 == sscanf(argv[i + 3], "%u", &g_normal.h)) {

			g_normal.filename = argv[i + 1];
			return 3;
		}
	}
	else
	if (i + 3 < argc && !strcmp(argv[i], arg_albedo)) {
		if (1 == sscanf(argv[i + 2], "%u", &g_albedo.w) &&
			1 == sscanf(argv[i + 3], "%u", &g_albedo.h)) {

			g_albedo.filename = argv[i + 1];
			return 3;
		}
	}
	else
	if (i + 1 < argc && !strcmp(argv[i], arg_anim_step)) {
		if (1 == sscanf(argv[i + 1], "%f", &g_anim_step) && 0.f < g_anim_step) {
			return 1;
		}
	}
	else
	if (i + 1 < argc && !strcmp(argv[i], arg_shadow_res)) {
		if (1 == sscanf(argv[i + 1], "%u", &g_fbo_res) && 0 == (g_fbo_res & g_fbo_res - 1)) {
			return 1;
		}
	}

	stream::cerr << "app options:\n"
		"\t" << arg_prefix << arg_app << " " << arg_normal <<
		" <filename> <width> <height>\t: use specified raw file and dimensions as source of normal map\n"
		"\t" << arg_prefix << arg_app << " " << arg_albedo <<
		" <filename> <width> <height>\t: use specified raw file and dimensions as source of albedo map\n"
		"\t" << arg_prefix << arg_app << " " << arg_anim_step <<
		" <step>\t\t\t\t: use specified animation step; entire animation is 1.0\n"
		"\t" << arg_prefix << arg_app << " " << arg_shadow_res <<
		" <pot>\t\t\t\t: use specified shadow buffer resolution (POT); default is " << fbo_default_res << "\n\n";

	return -1;
}

} // namespace

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

#if DRAW_SKELETON
template <>
inline bool
bindVertexBuffersAndPointers< PROG_SKEL >()
{
	glBindBuffer(GL_ARRAY_BUFFER, g_vbo[VBO_SKEL_VTX]);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

	DEBUG_GL_ERR()

	return st::setupVertexAttrPointers< st::Vertex >(g_active_attr_semantics[PROG_SKEL]);
}

#endif // DRAW_SKELETON
template <>
inline bool
bindVertexBuffersAndPointers< PROG_SHADOW >()
{
	glBindBuffer(GL_ARRAY_BUFFER, g_vbo[VBO_SKIN_VTX]);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, g_vbo[VBO_SKIN_IDX]);

	DEBUG_GL_ERR()

	return sk::setupVertexAttrPointers< sk::Vertex >(g_active_attr_semantics[PROG_SHADOW]);
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

	glDeleteFramebuffers(1, &g_fbo);
	g_fbo = 0;

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
	// set up various patches to the shaders

	std::ostringstream patch_res[2];
	patch_res[0] << "const float shadow_res = " << fbo_default_res << ".0;";
	patch_res[1] << "const float shadow_res = " << g_fbo_res << ".0;";

	const std::string patch[] = {
		patch_res[0].str(),
		patch_res[1].str()
	};

	/////////////////////////////////////////////////////////////////
	// set up misc control bits and values

	glEnable(GL_CULL_FACE);

	const GLclampf red = .25f;
	const GLclampf green = .25f;
	const GLclampf blue = .25f;
	const GLclampf alpha = 0.f;

	glClearColor(red, green, blue, alpha);
	glClearDepthf(1.f);

	// for shadow map generation: push back by 64 ULPs
	glPolygonOffset(1.f, 64.f);

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
	// create shader program SKIN from two shaders

	g_shader_vert[PROG_SKIN] = glCreateShader(GL_VERTEX_SHADER);
	assert(g_shader_vert[PROG_SKIN]);

	if (!util::setupShader(g_shader_vert[PROG_SKIN], "asset/shader/blinn_shadow_skinning.glslv")) {
		stream::cerr << __FUNCTION__ << " failed at setupShader\n";
		return false;
	}

	g_shader_frag[PROG_SKIN] = glCreateShader(GL_FRAGMENT_SHADER);
	assert(g_shader_frag[PROG_SKIN]);

	if (!util::setupShaderWithPatch(g_shader_frag[PROG_SKIN], "asset/shader/blinn_shadow.glslf",
			sizeof(patch) / sizeof(patch[0]) / 2, patch))
	{
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

	/////////////////////////////////////////////////////////////////
	// query the program about known uniform vars and vertex attribs

	g_uni[PROG_SKIN][UNI_MVP]     = glGetUniformLocation(g_shader_prog[PROG_SKIN], "mvp");
	g_uni[PROG_SKIN][UNI_MVP_LIT] = glGetUniformLocation(g_shader_prog[PROG_SKIN], "mvp_lit");
	g_uni[PROG_SKIN][UNI_BONE]    = glGetUniformLocation(g_shader_prog[PROG_SKIN], "bone");
	g_uni[PROG_SKIN][UNI_LP_OBJ]  = glGetUniformLocation(g_shader_prog[PROG_SKIN], "lp_obj");
	g_uni[PROG_SKIN][UNI_VP_OBJ]  = glGetUniformLocation(g_shader_prog[PROG_SKIN], "vp_obj");

	g_uni[PROG_SKIN][UNI_SAMPLER_NORMAL] = glGetUniformLocation(g_shader_prog[PROG_SKIN], "normal_map");
	g_uni[PROG_SKIN][UNI_SAMPLER_ALBEDO] = glGetUniformLocation(g_shader_prog[PROG_SKIN], "albedo_map");
	g_uni[PROG_SKIN][UNI_SAMPLER_SHADOW] = glGetUniformLocation(g_shader_prog[PROG_SKIN], "shadow_map");

	g_active_attr_semantics[PROG_SKIN].registerVertexAttr(glGetAttribLocation(g_shader_prog[PROG_SKIN], "at_Vertex"));
	g_active_attr_semantics[PROG_SKIN].registerNormalAttr(glGetAttribLocation(g_shader_prog[PROG_SKIN], "at_Normal"));
	g_active_attr_semantics[PROG_SKIN].registerBlendWAttr(glGetAttribLocation(g_shader_prog[PROG_SKIN], "at_Weight"));
	g_active_attr_semantics[PROG_SKIN].registerTCoordAttr(glGetAttribLocation(g_shader_prog[PROG_SKIN], "at_MultiTexCoord0"));

	/////////////////////////////////////////////////////////////////
	// create shader program SKEL from two shaders

	g_shader_vert[PROG_SKEL] = glCreateShader(GL_VERTEX_SHADER);
	assert(g_shader_vert[PROG_SKEL]);

	if (!util::setupShader(g_shader_vert[PROG_SKEL], "asset/shader/mvp.glslv")) {
		stream::cerr << __FUNCTION__ << " failed at setupShader\n";
		return false;
	}

	g_shader_frag[PROG_SKEL] = glCreateShader(GL_FRAGMENT_SHADER);
	assert(g_shader_frag[PROG_SKEL]);

	if (!util::setupShader(g_shader_frag[PROG_SKEL], "asset/shader/basic.glslf")) {
		stream::cerr << __FUNCTION__ << " failed at setupShader\n";
		return false;
	}

	g_shader_prog[PROG_SKEL] = glCreateProgram();
	assert(g_shader_prog[PROG_SKEL]);

	if (!util::setupProgram(
			g_shader_prog[PROG_SKEL],
			g_shader_vert[PROG_SKEL],
			g_shader_frag[PROG_SKEL]))
	{
		stream::cerr << __FUNCTION__ << " failed at setupProgram\n";
		return false;
	}

	/////////////////////////////////////////////////////////////////
	// query the program about known uniform vars and vertex attribs

	g_uni[PROG_SKEL][UNI_MVP]         = glGetUniformLocation(g_shader_prog[PROG_SKEL], "mvp");
	g_uni[PROG_SKEL][UNI_SOLID_COLOR] = glGetUniformLocation(g_shader_prog[PROG_SKEL], "solid_color");

	g_active_attr_semantics[PROG_SKEL].registerVertexAttr(glGetAttribLocation(g_shader_prog[PROG_SKEL], "at_Vertex"));

	/////////////////////////////////////////////////////////////////
	// create shader program SHADOW from two shaders

	g_shader_vert[PROG_SHADOW] = glCreateShader(GL_VERTEX_SHADER);
	assert(g_shader_vert[PROG_SHADOW]);

	if (!util::setupShader(g_shader_vert[PROG_SHADOW], "asset/shader/mvp_skinning.glslv")) {
		stream::cerr << __FUNCTION__ << " failed at setupShader\n";
		return false;
	}

	g_shader_frag[PROG_SHADOW] = glCreateShader(GL_FRAGMENT_SHADER);
	assert(g_shader_frag[PROG_SHADOW]);

	if (!util::setupShader(g_shader_frag[PROG_SHADOW], "asset/shader/depth.glslf")) {
		stream::cerr << __FUNCTION__ << " failed at setupShader\n";
		return false;
	}

	g_shader_prog[PROG_SHADOW] = glCreateProgram();
	assert(g_shader_prog[PROG_SHADOW]);

	if (!util::setupProgram(
			g_shader_prog[PROG_SHADOW],
			g_shader_vert[PROG_SHADOW],
			g_shader_frag[PROG_SHADOW]))
	{
		stream::cerr << __FUNCTION__ << " failed at setupProgram\n";
		return false;
	}

	/////////////////////////////////////////////////////////////////
	// query the program about known uniform vars and vertex attribs

	g_uni[PROG_SHADOW][UNI_MVP]  = glGetUniformLocation(g_shader_prog[PROG_SHADOW], "mvp");
	g_uni[PROG_SHADOW][UNI_BONE] = glGetUniformLocation(g_shader_prog[PROG_SHADOW], "bone");

	g_active_attr_semantics[PROG_SHADOW].registerVertexAttr(glGetAttribLocation(g_shader_prog[PROG_SHADOW], "at_Vertex"));
	g_active_attr_semantics[PROG_SHADOW].registerBlendWAttr(glGetAttribLocation(g_shader_prog[PROG_SHADOW], "at_Weight"));

	/////////////////////////////////////////////////////////////////
	// load the skeleton for the main geometric asset

	g_bone_count = BONE_CAPACITY;

	const char* const skeleton_filename = "asset/mesh/Ahmed_GEO.skeleton";

	if (!rend::loadSkeletonAnimationABE(skeleton_filename, &g_bone_count, g_bone_mat, g_bone, g_animations, g_durations)) {
		stream::cerr << __FUNCTION__ << " failed to load skeleton file " << skeleton_filename << '\n';
		return false;
	}

	assert(g_animations.size());
	assert(g_durations.size());

	anim::at = g_animations.begin();
	anim::dt = g_durations.begin();
	anim::animTime = 0.f;
	anim::duration = *anim::dt;

	/////////////////////////////////////////////////////////////////
	// reserve VAO (if available) and all necessary VBOs

#if PLATFORM_GL_OES_vertex_array_object
	glGenVertexArraysOES(sizeof(g_vao) / sizeof(g_vao[0]), g_vao);

	for (unsigned i = 0; i < sizeof(g_vao) / sizeof(g_vao[0]); ++i)
		assert(g_vao[i]);

#endif
	glGenBuffers(sizeof(g_vbo) / sizeof(g_vbo[0]), g_vbo);

	for (unsigned i = 0; i < sizeof(g_vbo) / sizeof(g_vbo[0]); ++i)
		assert(g_vbo[i]);

	/////////////////////////////////////////////////////////////////
	// load the main geometric asset

	float bbox_min[3];
	float bbox_max[3];

	const uintptr_t semantics_offset[4] = {
		offsetof(sk::Vertex, pos),
		offsetof(sk::Vertex, bon),
		offsetof(sk::Vertex, nrm),
		offsetof(sk::Vertex, txc)
	};

	const char* const mesh_filename = "asset/mesh/Ahmed_GEO.mesh";

	if (!util::fill_indexed_trilist_from_file_ABE(
			mesh_filename,
			g_vbo[VBO_SKIN_VTX],
			g_vbo[VBO_SKIN_IDX],
			semantics_offset,
			g_num_faces[MESH_SKIN],
			g_index_type,
			bbox_min,
			bbox_max))
	{
		stream::cerr << __FUNCTION__ << " failed at fill_indexed_trilist_from_file_ABE\n";
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

#if DRAW_SKELETON
	/////////////////////////////////////////////////////////////////
	// forward-declare VBO_SKEL_VTX; dynamically updated per frame

	glBindBuffer(GL_ARRAY_BUFFER, g_vbo[VBO_SKEL_VTX]);
	glBufferData(GL_ARRAY_BUFFER, sizeof(g_stick[0]) * g_bone_count, NULL, GL_DYNAMIC_DRAW);
	glBindBuffer(GL_ARRAY_BUFFER, 0);

#endif
#if PLATFORM_GL_OES_vertex_array_object
	glBindVertexArrayOES(g_vao[PROG_SKIN]);

	if (!bindVertexBuffersAndPointers< PROG_SKIN >() || (DEBUG_LITERAL && util::reportGLError())) {
		stream::cerr << __FUNCTION__ << "failed at bindVertexBuffersAndPointers for PROG_SKIN\n";
		return false;
	}

	for (unsigned i = 0; i < g_active_attr_semantics[PROG_SKIN].num_active_attr; ++i)
		glEnableVertexAttribArray(g_active_attr_semantics[PROG_SKIN].active_attr[i]);

	DEBUG_GL_ERR()

#if DRAW_SKELETON
	glBindVertexArrayOES(g_vao[PROG_SKEL]);

	if (!bindVertexBuffersAndPointers< PROG_SKEL >() || (DEBUG_LITERAL && util::reportGLError())) {
		stream::cerr << __FUNCTION__ << "failed at bindVertexBuffersAndPointers for PROG_SKEL\n";
		return false;
	}

	for (unsigned i = 0; i < g_active_attr_semantics[PROG_SKEL].num_active_attr; ++i)
		glEnableVertexAttribArray(g_active_attr_semantics[PROG_SKEL].active_attr[i]);

	DEBUG_GL_ERR()

#endif
	glBindVertexArrayOES(g_vao[PROG_SHADOW]);

	if (!bindVertexBuffersAndPointers< PROG_SHADOW >() || (DEBUG_LITERAL && util::reportGLError())) {
		stream::cerr << __FUNCTION__ << "failed at bindVertexBuffersAndPointers for PROG_SHADOW\n";
		return false;
	}

	for (unsigned i = 0; i < g_active_attr_semantics[PROG_SHADOW].num_active_attr; ++i)
		glEnableVertexAttribArray(g_active_attr_semantics[PROG_SHADOW].active_attr[i]);

	DEBUG_GL_ERR()

	glBindVertexArrayOES(0);

#endif
	/////////////////////////////////////////////////////////////////
	// set up the texture to be used as depth in the single FBO

	glBindTexture(GL_TEXTURE_2D, g_tex[TEX_SHADOW]);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

#if DEPTH_PRECISION_24
	glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_STENCIL_OES, g_fbo_res, g_fbo_res, 0,
		GL_DEPTH_STENCIL_OES, GL_UNSIGNED_INT_24_8_OES, 0);
#else
	glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, g_fbo_res, g_fbo_res, 0,
		GL_DEPTH_COMPONENT, GL_UNSIGNED_SHORT, 0);
#endif

	if (util::reportGLError()) {
		stream::cerr << __FUNCTION__ << " failed at shadow texture setup\n";
		return false;
	}

	glBindTexture(GL_TEXTURE_2D, 0);

	/////////////////////////////////////////////////////////////////
	// set up the single FBO

	glGenFramebuffers(1, &g_fbo);
	assert(g_fbo);

	glBindFramebuffer(GL_FRAMEBUFFER, g_fbo);

	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, g_tex[TEX_SHADOW], 0);

#if DEPTH_PRECISION_24
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_STENCIL_ATTACHMENT, GL_TEXTURE_2D, g_tex[TEX_SHADOW], 0);

#endif
	const GLenum fbo_success = glCheckFramebufferStatus(GL_FRAMEBUFFER);

	if (GL_FRAMEBUFFER_COMPLETE != fbo_success) {
		stream::cerr << __FUNCTION__ << " failed at glCheckFramebufferStatus\n";
		return false;
	}

	glDrawBuffers(0, nullptr);

	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	on_error.reset();
	return true;
}

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

bool
hook::render_frame(GLuint primary_fbo)
{
	if (!check_context(__FUNCTION__))
		return false;

	/////////////////////////////////////////////////////////////////
	// fast-forward the skeleton animation to current time

	while (anim::duration <= anim::animTime) {
		if (g_animations.end() == ++anim::at)
			anim::at = g_animations.begin();

		if (g_durations.end() == ++anim::dt)
			anim::dt = g_durations.begin();

		anim::animTime -= anim::duration;
		anim::duration = *anim::dt;
		rend::resetSkeletonAnimProgress(*anim::at);
	}

	rend::animateSkeleton(g_bone_count, g_bone_mat, g_bone, *anim::at, anim::animTime, g_root_bone);
	anim::animTime += g_anim_step;

#if DRAW_SKELETON
	/////////////////////////////////////////////////////////////////
	// produce line segments from the bones of the animated skeleton

	assert(255 == g_bone[0].parent_idx);

	for (unsigned i = 1; i < g_bone_count; ++i) {
		const unsigned j = g_bone[i].parent_idx;
		const simd::vect4::basetype& pos0 = 255 != j ? g_bone[j].to_model[3] : g_root_bone->to_model[3]; // accommodate multi-root skeletons
		const simd::vect4::basetype& pos1 = g_bone[i].to_model[3];

		g_stick[i][0].pos[0] = pos0[0];
		g_stick[i][0].pos[1] = pos0[1];
		g_stick[i][0].pos[2] = pos0[2];
		g_stick[i][1].pos[0] = pos1[0];
		g_stick[i][1].pos[1] = pos1[1];
		g_stick[i][1].pos[2] = pos1[2];
	}

#endif
	/////////////////////////////////////////////////////////////////
	// compute two MVPs: one for screen space and one for light space

	const float z_offset = -2.125f;
	const float shadow_extent = 1.5f;
	const simd::vect4 translate(0.f, 0.f, z_offset, 0.f);

	simd::matx4 mv = simd::matx4().mul(g_root_bone->to_model, g_matx_fit);
	mv.set(3, simd::vect4().add(mv[3], translate));

	const float l = -.5f;
	const float r = .5f;
	const float b = -.5f;
	const float t = .5f;
	const float n = 1.f;
	const float f = 4.f;

	const matx4_persp proj(l, r, b, t, n, f);

	const matx4_ortho proj_lit(
		-shadow_extent, shadow_extent,
		-shadow_extent, shadow_extent, 0.f, 4.f);

	// light-space basis vectors
	const simd::vect3 x_lit(0.707107, 0.0, -0.707107); // x-axis rotated at pi/4 around y-axis
	const simd::vect3 y_lit(0.0, 1.0, 0.0);            // y-axis
	const simd::vect3 z_lit(0.707107, 0.0, 0.707107);  // z-axis rotated at pi/4 around y-axis

	const simd::matx4 world_lit(
		x_lit[0], x_lit[1], x_lit[2], 0.f,
		y_lit[0], y_lit[1], y_lit[2], 0.f,
		z_lit[0], z_lit[1], z_lit[2], 0.f,
		z_lit[0] * 2.f,
		z_lit[1] * 2.f,
		z_lit[2] * 2.f + z_offset, 1.f);

	const simd::matx4 local_lit = simd::matx4().inverse(world_lit);
	const simd::matx4 viewproj_lit = simd::matx4().mul(local_lit, proj_lit);

	const float depth_compensation = 1.f / 1024.f; // slope-invariant compensation
	const simd::matx4 bias(
		.5f, 0.f, 0.f, 0.f,
		0.f, .5f, 0.f, 0.f,
		0.f, 0.f, .5f, 0.f,
		.5f, .5f, .5f - depth_compensation, 1.f);

	const simd::matx4 mvp =
		simd::matx4().mul(mv, proj);

	const simd::matx4 mvp_lit =
		simd::matx4().mul(mv, viewproj_lit);

	const simd::matx4 biased_lit =
		simd::matx4().mul(mv, viewproj_lit).mulr(bias);

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

	const rend::dense_matx4 dense_mvp_lit = rend::dense_matx4(
			mvp_lit[0][0], mvp_lit[0][1], mvp_lit[0][2], mvp_lit[0][3],
			mvp_lit[1][0], mvp_lit[1][1], mvp_lit[1][2], mvp_lit[1][3],
			mvp_lit[2][0], mvp_lit[2][1], mvp_lit[2][2], mvp_lit[2][3],
			mvp_lit[3][0], mvp_lit[3][1], mvp_lit[3][2], mvp_lit[3][3]);

	const rend::dense_matx4 dense_biased_lit = rend::dense_matx4(
			biased_lit[0][0], biased_lit[0][1], biased_lit[0][2], biased_lit[0][3],
			biased_lit[1][0], biased_lit[1][1], biased_lit[1][2], biased_lit[1][3],
			biased_lit[2][0], biased_lit[2][1], biased_lit[2][2], biased_lit[2][3],
			biased_lit[3][0], biased_lit[3][1], biased_lit[3][2], biased_lit[3][3]);

	/////////////////////////////////////////////////////////////////
	// render the shadow of the geometric asset into the FBO

	glBindFramebuffer(GL_FRAMEBUFFER, g_fbo);
	glViewport(0, 0, g_fbo_res, g_fbo_res);

	DEBUG_GL_ERR()

	glEnable(GL_DEPTH_TEST);
	glEnable(GL_POLYGON_OFFSET_FILL);

	glClear(GL_DEPTH_BUFFER_BIT);

	glUseProgram(g_shader_prog[PROG_SHADOW]);

	DEBUG_GL_ERR()

	if (-1 != g_uni[PROG_SHADOW][UNI_MVP]) {
		glUniformMatrix4fv(g_uni[PROG_SHADOW][UNI_MVP],
			1, GL_FALSE, static_cast< const GLfloat* >(dense_mvp_lit));

		DEBUG_GL_ERR()
	}

	if (-1 != g_uni[PROG_SHADOW][UNI_BONE]) {
		glUniformMatrix4fv(g_uni[PROG_SHADOW][UNI_BONE],
			g_bone_count, GL_FALSE, static_cast< const GLfloat* >(g_bone_mat[0]));

		DEBUG_GL_ERR()
	}

#if PLATFORM_GL_OES_vertex_array_object
	glBindVertexArrayOES(g_vao[PROG_SHADOW]);

	DEBUG_GL_ERR()

#else
	if (!bindVertexBuffersAndPointers< PROG_SHADOW >())
		return false;

	for (unsigned i = 0; i < g_active_attr_semantics[PROG_SHADOW].num_active_attr; ++i)
		glEnableVertexAttribArray(g_active_attr_semantics[PROG_SHADOW].active_attr[i]);

	DEBUG_GL_ERR()

#endif
	glDrawElements(GL_TRIANGLES, g_num_faces[MESH_SKIN] * 3, g_index_type, 0);

	DEBUG_GL_ERR()

#if PLATFORM_GL_OES_vertex_array_object == 0
	for (unsigned i = 0; i < g_active_attr_semantics[PROG_SHADOW].num_active_attr; ++i)
		glDisableVertexAttribArray(g_active_attr_semantics[PROG_SHADOW].active_attr[i]);

	DEBUG_GL_ERR()

#endif
	/////////////////////////////////////////////////////////////////
	// render the geometric asset into the main framebuffer

	glBindFramebuffer(GL_FRAMEBUFFER, primary_fbo);
	glViewport(vp[0], vp[1], vp[2], vp[3]);

	DEBUG_GL_ERR()

	glDisable(GL_POLYGON_OFFSET_FILL);

	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glUseProgram(g_shader_prog[PROG_SKIN]);

	DEBUG_GL_ERR()

	if (-1 != g_uni[PROG_SKIN][UNI_MVP]) {
		glUniformMatrix4fv(g_uni[PROG_SKIN][UNI_MVP],
			1, GL_FALSE, static_cast< const GLfloat* >(dense_mvp));

		DEBUG_GL_ERR()
	}

	if (-1 != g_uni[PROG_SKIN][UNI_MVP_LIT]) {
		glUniformMatrix4fv(g_uni[PROG_SKIN][UNI_MVP_LIT],
			1, GL_FALSE, static_cast< const GLfloat* >(dense_biased_lit));

		DEBUG_GL_ERR()
	}

	if (-1 != g_uni[PROG_SKIN][UNI_BONE]) {
		glUniformMatrix4fv(g_uni[PROG_SKIN][UNI_BONE],
			g_bone_count, GL_FALSE, static_cast< const GLfloat* >(g_bone_mat[0]));

		DEBUG_GL_ERR()
	}

	if (-1 != g_uni[PROG_SKIN][UNI_LP_OBJ]) {
		const GLfloat nonlocal_light[4] = {
			lp_obj[0],
			lp_obj[1],
			lp_obj[2],
			0.f
		};

		glUniform4fv(g_uni[PROG_SKIN][UNI_LP_OBJ], 1, nonlocal_light);

		DEBUG_GL_ERR()
	}

	if (-1 != g_uni[PROG_SKIN][UNI_VP_OBJ]) {
		const GLfloat nonlocal_viewer[4] = {
			mv[0][2],
			mv[1][2],
			mv[2][2],
			0.f
		};

		glUniform4fv(g_uni[PROG_SKIN][UNI_VP_OBJ], 1, nonlocal_viewer);

		DEBUG_GL_ERR()
	}

	if (0 != g_tex[TEX_NORMAL] && -1 != g_uni[PROG_SKIN][UNI_SAMPLER_NORMAL]) {
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, g_tex[TEX_NORMAL]);

		glUniform1i(g_uni[PROG_SKIN][UNI_SAMPLER_NORMAL], 0);

		DEBUG_GL_ERR()
	}

	if (0 != g_tex[TEX_ALBEDO] && -1 != g_uni[PROG_SKIN][UNI_SAMPLER_ALBEDO]) {
		glActiveTexture(GL_TEXTURE1);
		glBindTexture(GL_TEXTURE_2D, g_tex[TEX_ALBEDO]);

		glUniform1i(g_uni[PROG_SKIN][UNI_SAMPLER_ALBEDO], 1);

		DEBUG_GL_ERR()
	}

	if (0 != g_tex[TEX_SHADOW] && -1 != g_uni[PROG_SKIN][UNI_SAMPLER_SHADOW]) {
		glActiveTexture(GL_TEXTURE2);
		glBindTexture(GL_TEXTURE_2D, g_tex[TEX_SHADOW]);

		glUniform1i(g_uni[PROG_SKIN][UNI_SAMPLER_SHADOW], 2);

		DEBUG_GL_ERR()
	}

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
#if DRAW_SKELETON
	/////////////////////////////////////////////////////////////////
	// render the skeleton into the main framebuffer

	glDisable(GL_DEPTH_TEST);

	glUseProgram(g_shader_prog[PROG_SKEL]);

	DEBUG_GL_ERR()

	if (-1 != g_uni[PROG_SKEL][UNI_MVP]) {
		glUniformMatrix4fv(g_uni[PROG_SKEL][UNI_MVP],
			1, GL_FALSE, static_cast< const GLfloat* >(dense_mvp));

		DEBUG_GL_ERR()
	}

	if (-1 != g_uni[PROG_SKEL][UNI_SOLID_COLOR]) {
		const GLfloat solid_color[4] = { 0.f, 1.f, 0.f, 1.f };

		glUniform4fv(g_uni[PROG_SKEL][UNI_SOLID_COLOR], 1, solid_color);

		DEBUG_GL_ERR()
	}

#if PLATFORM_GL_OES_vertex_array_object
	glBindVertexArrayOES(g_vao[PROG_SKEL]);

	DEBUG_GL_ERR()

#else
	if (!bindVertexBuffersAndPointers< PROG_SKEL >())
		return false;

	for (unsigned i = 0; i < g_active_attr_semantics[PROG_SKEL].num_active_attr; ++i)
		glEnableVertexAttribArray(g_active_attr_semantics[PROG_SKEL].active_attr[i]);

	DEBUG_GL_ERR()

#endif
	glBindBuffer(GL_ARRAY_BUFFER, g_vbo[VBO_SKEL_VTX]);
	glBufferSubData(GL_ARRAY_BUFFER, GLintptr(0), sizeof(g_stick[0]) * g_bone_count, g_stick);

	DEBUG_GL_ERR()

	glDrawArrays(GL_LINES, 0, g_bone_count * 2);

	DEBUG_GL_ERR()

#if PLATFORM_GL_OES_vertex_array_object == 0
	for (unsigned i = 0; i < g_active_attr_semantics[PROG_SKEL].num_active_attr; ++i)
		glDisableVertexAttribArray(g_active_attr_semantics[PROG_SKEL].active_attr[i]);

	DEBUG_GL_ERR()

#endif
#endif // DRAW_SKELETON
	return true;
}
