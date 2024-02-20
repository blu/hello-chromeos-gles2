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
#include <cmath>

#include "scoped.hpp"
#include "stream.hpp"
#include "vectsimd.hpp"
#include "util_tex.hpp"
#include "util_misc.hpp"
#include "pure_macro.hpp"

#include "rendSkeleton.hpp"
#include "rendVertAttr.hpp"

using util::scoped_ptr;
using util::scoped_functor;
using util::deinit_resources_t;

namespace { // anonymous

#define SETUP_VERTEX_ATTR_POINTERS_MASK	( \
		SETUP_VERTEX_ATTR_POINTERS_MASK_vertex | \
		SETUP_VERTEX_ATTR_POINTERS_MASK_normal | \
		SETUP_VERTEX_ATTR_POINTERS_MASK_blendw | \
		SETUP_VERTEX_ATTR_POINTERS_MASK_tcoord)

#include "rendVertAttr_setupVertAttrPointers.hpp"
#undef SETUP_VERTEX_ATTR_POINTERS_MASK

struct Vertex {
	GLfloat pos[3];
	GLfloat nrm[3];
	GLfloat	bon[4];
	GLfloat txc[2];
};

const char arg_prefix[]    = "--";
const char arg_app[]       = "app";

const char arg_normal[]    = "normal_map";
const char arg_albedo[]    = "albedo_map";
const char arg_alt_anim[]  = "alt_anim";
const char arg_anim_step[] = "anim_step";

struct TexDesc {
	const char* filename;
	unsigned w;
	unsigned h;
};

TexDesc g_normal = { "asset/texture/chesterfield.raw", 1024, 1024 };
TexDesc g_albedo = { "asset/texture/navy_blue_scrapbook_paper.raw", 512, 512 };

const unsigned bone_count = 13;

rend::Bone g_bone[bone_count];
rend::dense_matx4 g_bone_mat[bone_count];
std::vector< rend::Track > g_skeletal_animation;
float g_anim_step = .0125f;
bool g_alt_anim;

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
	PROG_SKIN,

	PROG_COUNT,
	PROG_FORCE_UINT = -1U
};

enum {
	UNI_SAMPLER_NORMAL,
	UNI_SAMPLER_ALBEDO,

	UNI_LP_OBJ,
	UNI_VP_OBJ,
	UNI_BONE,
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
	if (!strcmp(argv[i], arg_alt_anim)) {
		g_alt_anim = true;
		return 0;
	}

	stream::cerr << "app options:\n"
		"\t" << arg_prefix << arg_app << " " << arg_normal <<
		" <filename> <width> <height>\t: use specified raw file and dimensions as source of normal map\n"
		"\t" << arg_prefix << arg_app << " " << arg_albedo <<
		" <filename> <width> <height>\t: use specified raw file and dimensions as source of albedo map\n"
		"\t" << arg_prefix << arg_app << " " << arg_alt_anim <<
		"\t\t\t\t\t: use alternative skeleton animation\n"
		"\t" << arg_prefix << arg_app << " " << arg_anim_step <<
		" <step>\t\t\t\t: use specified animation step; entire animation is 1.0\n\n";

	return -1;
}

} // namespace

namespace { // anonymous

// set up skeleton animation, type A
void setupSkeletonAnimA()
{
	const simd::quat identq(0.f, 0.f, 0.f, 1.f);

	rend::Track& track = *g_skeletal_animation.insert(g_skeletal_animation.end(), rend::Track());
	track.bone_idx = 0;

	rend::BoneOrientationKey& ori0 =
		*track.orientation_key.insert(track.orientation_key.end(), rend::BoneOrientationKey());

	ori0.time = 0.f;
	ori0.value = identq;

	rend::BoneOrientationKey& ori1 =
		*track.orientation_key.insert(track.orientation_key.end(), rend::BoneOrientationKey());

	ori1.time = .25f;
	ori1.value = simd::quat(M_PI * .25f, simd::vect3(1.f, 1.f, 1.f).normalise());

	rend::BoneOrientationKey& ori2 =
		*track.orientation_key.insert(track.orientation_key.end(), rend::BoneOrientationKey());

	ori2.time = .75f;
	ori2.value = simd::quat(M_PI * -.25f, simd::vect3(1.f, 1.f, 1.f).normalise());

	rend::BoneOrientationKey& ori3 =
		*track.orientation_key.insert(track.orientation_key.end(), rend::BoneOrientationKey());

	ori3.time = 1.f;
	ori3.value = identq;

	track.position_last_key_idx = 0;
	track.orientation_last_key_idx = 0;
	track.scale_last_key_idx = 0;

	static const uint8_t anim_bone[] =
	{
		4, 5, 10, 11
	};

	for (unsigned i = 0; i < sizeof(anim_bone) / sizeof(anim_bone[0]); ++i)
	{
		rend::Track& track = *g_skeletal_animation.insert(g_skeletal_animation.end(), rend::Track());
		track.bone_idx = anim_bone[i];

		rend::BoneOrientationKey& ori0 =
			*track.orientation_key.insert(track.orientation_key.end(), rend::BoneOrientationKey());

		ori0.time = 0.f;
		ori0.value = identq;

		rend::BoneOrientationKey& ori1 =
			*track.orientation_key.insert(track.orientation_key.end(), rend::BoneOrientationKey());

		ori1.time = .25f;
		ori1.value = simd::quat(M_PI * .375f, simd::vect3(0.f, 1.f, 0.f));

		rend::BoneOrientationKey& ori2 =
			*track.orientation_key.insert(track.orientation_key.end(), rend::BoneOrientationKey());

		ori2.time = .5f;
		ori2.value = identq;

		rend::BoneOrientationKey& ori3 =
			*track.orientation_key.insert(track.orientation_key.end(), rend::BoneOrientationKey());

		ori3.time = .75f;
		ori3.value = simd::quat(M_PI * -.375f, simd::vect3(0.f, 1.f, 0.f));

		rend::BoneOrientationKey& ori4 =
			*track.orientation_key.insert(track.orientation_key.end(), rend::BoneOrientationKey());

		ori4.time = 1.f;
		ori4.value = identq;

		track.position_last_key_idx = 0;
		track.orientation_last_key_idx = 0;
		track.scale_last_key_idx = 0;
	}
}

// set up skeleton animation, type B
void setupSkeletonAnimB()
{
	const simd::quat identq(0.f, 0.f, 0.f, 1.f);

	rend::Track& track = *g_skeletal_animation.insert(g_skeletal_animation.end(), rend::Track());
	track.bone_idx = 0;

	rend::BoneOrientationKey& ori0 =
		*track.orientation_key.insert(track.orientation_key.end(), rend::BoneOrientationKey());

	ori0.time = 0.f;
	ori0.value = identq;

	rend::BoneOrientationKey& ori1 =
		*track.orientation_key.insert(track.orientation_key.end(), rend::BoneOrientationKey());

	ori1.time = .25f;
	ori1.value = simd::quat(M_PI * .5f, simd::vect3(1.f, 1.f, 0.f).normalise());

	rend::BoneOrientationKey& ori2 =
		*track.orientation_key.insert(track.orientation_key.end(), rend::BoneOrientationKey());

	ori2.time = .5f;
	ori2.value = simd::quat(M_PI, simd::vect3(1.f, 1.f, 0.f).normalise());

	rend::BoneOrientationKey& ori3 =
		*track.orientation_key.insert(track.orientation_key.end(), rend::BoneOrientationKey());

	ori3.time = .75f;
	ori3.value = simd::quat(M_PI * 1.5f, simd::vect3(1.f, 1.f, 0.f).normalise());

	rend::BoneOrientationKey& ori4 =
		*track.orientation_key.insert(track.orientation_key.end(), rend::BoneOrientationKey());

	ori4.time = 1.f;
	ori4.value = simd::quat(M_PI * 2.f, simd::vect3(1.f, 1.f, 0.f).normalise());

	track.position_last_key_idx = 0;
	track.orientation_last_key_idx = 0;
	track.scale_last_key_idx = 0;

	static const uint8_t anim_bone[] =
	{
		1, 2, 4, 5, 7, 8, 10, 11
	};

	for (unsigned i = 0; i < sizeof(anim_bone) / sizeof(anim_bone[0]); ++i)
	{
		const simd::vect3& axis = i % 4 & 2
			? simd::vect3(0.f, 1.f, 0.f)
			: simd::vect3(1.f, 0.f, 0.f);

		const float angle = 1 == i / 2 || 2 == i / 2
			? -.375f
			: .375f;

		rend::Track& track = *g_skeletal_animation.insert(g_skeletal_animation.end(), rend::Track());
		track.bone_idx = anim_bone[i];

		rend::BoneOrientationKey& ori0 =
			*track.orientation_key.insert(track.orientation_key.end(), rend::BoneOrientationKey());

		ori0.time = 0.f;
		ori0.value = identq;

		rend::BoneOrientationKey& ori1 =
			*track.orientation_key.insert(track.orientation_key.end(), rend::BoneOrientationKey());

		ori1.time = .25f;
		ori1.value = simd::quat(M_PI * angle, axis);

		rend::BoneOrientationKey& ori2 =
			*track.orientation_key.insert(track.orientation_key.end(), rend::BoneOrientationKey());

		ori2.time = .5f;
		ori2.value = identq;

		rend::BoneOrientationKey& ori3 =
			*track.orientation_key.insert(track.orientation_key.end(), rend::BoneOrientationKey());

		ori3.time = .75f;
		ori3.value = simd::quat(-M_PI * angle, axis);

		rend::BoneOrientationKey& ori4 =
			*track.orientation_key.insert(track.orientation_key.end(), rend::BoneOrientationKey());

		ori4.time = 1.f;
		ori4.value = identq;

		track.position_last_key_idx = 0;
		track.orientation_last_key_idx = 0;
		track.scale_last_key_idx = 0;
	}
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

bool init_resources(
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
	// set up misc control bits and values

	glDisable(GL_CULL_FACE);
	glEnable(GL_DEPTH_TEST);

	const GLclampf red = .25f;
	const GLclampf green = .25f;
	const GLclampf blue = .25f;
	const GLclampf alpha = 1.f;

	glClearColor(red, green, blue, alpha);
	glClearDepthf(1.f);

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

	g_shader_vert[PROG_SKIN] = glCreateShader(GL_VERTEX_SHADER);
	assert(g_shader_vert[PROG_SKIN]);

	if (!util::setupShader(g_shader_vert[PROG_SKIN], "asset/shader/phong_skinning_bump_tang.glslv")) {
		stream::cerr << __FUNCTION__ << " failed at setupShader\n";
		return false;
	}

	g_shader_frag[PROG_SKIN] = glCreateShader(GL_FRAGMENT_SHADER);
	assert(g_shader_frag[PROG_SKIN]);

	if (!util::setupShader(g_shader_frag[PROG_SKIN], "asset/shader/phong_bump_tang.glslf")) {
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

	g_uni[PROG_SKIN][UNI_MVP]    = glGetUniformLocation(g_shader_prog[PROG_SKIN], "mvp");
	g_uni[PROG_SKIN][UNI_BONE]   = glGetUniformLocation(g_shader_prog[PROG_SKIN], "bone");
	g_uni[PROG_SKIN][UNI_LP_OBJ] = glGetUniformLocation(g_shader_prog[PROG_SKIN], "lp_obj");
	g_uni[PROG_SKIN][UNI_VP_OBJ] = glGetUniformLocation(g_shader_prog[PROG_SKIN], "vp_obj");

	g_uni[PROG_SKIN][UNI_SAMPLER_NORMAL] = glGetUniformLocation(g_shader_prog[PROG_SKIN], "normal_map");
	g_uni[PROG_SKIN][UNI_SAMPLER_ALBEDO] = glGetUniformLocation(g_shader_prog[PROG_SKIN], "albedo_map");

	g_active_attr_semantics[PROG_SKIN].registerVertexAttr(glGetAttribLocation(g_shader_prog[PROG_SKIN], "at_Vertex"));
	g_active_attr_semantics[PROG_SKIN].registerNormalAttr(glGetAttribLocation(g_shader_prog[PROG_SKIN], "at_Normal"));
	g_active_attr_semantics[PROG_SKIN].registerBlendWAttr(glGetAttribLocation(g_shader_prog[PROG_SKIN], "at_Weight"));
	g_active_attr_semantics[PROG_SKIN].registerTCoordAttr(glGetAttribLocation(g_shader_prog[PROG_SKIN], "at_MultiTexCoord0"));

	/////////////////////////////////////////////////////////////////
	// set up skeleton in binding pose

	const float bolen = .25f;

	const simd::quat identq(0.f, 0.f, 0.f, 1.f);
	const simd::vect3 idents(1.f, 1.f, 1.f);

	g_bone[0].position = simd::vect3(0.f, 0.f, 0.f);
	g_bone[0].orientation = identq;
	g_bone[0].scale = idents;
	g_bone[0].parent_idx = 255;

	g_bone[1].position = simd::vect3(0.f, bolen, 0.f);
	g_bone[1].orientation = identq;
	g_bone[1].scale = idents;
	g_bone[1].parent_idx = 0;

	g_bone[2].position = simd::vect3(0.f, bolen, 0.f);
	g_bone[2].orientation = identq;
	g_bone[2].scale = idents;
	g_bone[2].parent_idx = 1;

	g_bone[3].position = simd::vect3(0.f, bolen, 0.f);
	g_bone[3].orientation = identq;
	g_bone[3].scale = idents;
	g_bone[3].parent_idx = 2;

	g_bone[4].position = simd::vect3(-bolen, 0.f, 0.f);
	g_bone[4].orientation = identq;
	g_bone[4].scale = idents;
	g_bone[4].parent_idx = 0;

	g_bone[5].position = simd::vect3(-bolen, 0.f, 0.f);
	g_bone[5].orientation = identq;
	g_bone[5].scale = idents;
	g_bone[5].parent_idx = 4;

	g_bone[6].position = simd::vect3(-bolen, 0.f, 0.f);
	g_bone[6].orientation = identq;
	g_bone[6].scale = idents;
	g_bone[6].parent_idx = 5;

	g_bone[7].position = simd::vect3(0.f, -bolen, 0.f);
	g_bone[7].orientation = identq;
	g_bone[7].scale = idents;
	g_bone[7].parent_idx = 0;

	g_bone[8].position = simd::vect3(0.f, -bolen, 0.f);
	g_bone[8].orientation = identq;
	g_bone[8].scale = idents;
	g_bone[8].parent_idx = 7;

	g_bone[9].position = simd::vect3(0.f, -bolen, 0.f);
	g_bone[9].orientation = identq;
	g_bone[9].scale = idents;
	g_bone[9].parent_idx = 8;

	g_bone[10].position = simd::vect3(bolen, 0.f, 0.f);
	g_bone[10].orientation = identq;
	g_bone[10].scale = idents;
	g_bone[10].parent_idx = 0;

	g_bone[11].position = simd::vect3(bolen, 0.f, 0.f);
	g_bone[11].orientation = identq;
	g_bone[11].scale = idents;
	g_bone[11].parent_idx = 10;

	g_bone[12].position = simd::vect3(bolen, 0.f, 0.f);
	g_bone[12].orientation = identq;
	g_bone[12].scale = idents;
	g_bone[12].parent_idx = 11;

	for (unsigned i = 0; i < bone_count; ++i)
		rend::initBoneMatx(bone_count, g_bone_mat, g_bone, i);

	if (g_alt_anim)
		setupSkeletonAnimB();
	else
		setupSkeletonAnimA();

	/////////////////////////////////////////////////////////////////
	// set up mesh of { pos, nrm, bon, txc }

	static const Vertex arr[] =
	{
		{ { 0.f, 0.f, 0.f }, { 0.f, 0.f, 1.f }, { 1.f, 0.f, 0.f, 0.f }, { .5f, .5f } },

		// north sleeve
		{ { bolen, 1.f * bolen, 0.f }, { 0.f, 0.f, 1.f }, { 1.f, 0.f, 0.f, 0.f }, { .5f + .5f / 3.f, .5f + .5f / 3.f } },
		{ {-bolen, 1.f * bolen, 0.f }, { 0.f, 0.f, 1.f }, { 1.f, 0.f, 0.f, 0.f }, { .5f - .5f / 3.f, .5f + .5f / 3.f } },
		{ { bolen, 2.f * bolen, 0.f }, { 0.f, 0.f, 1.f }, { 1.f, 0.f, 0.f, 2.f }, { .5f + .5f / 3.f, .5f + 1.f / 3.f } },
		{ {-bolen, 2.f * bolen, 0.f }, { 0.f, 0.f, 1.f }, { 1.f, 0.f, 0.f, 2.f }, { .5f - .5f / 3.f, .5f + 1.f / 3.f } },
		{ { bolen, 3.f * bolen, 0.f }, { 0.f, 0.f, 1.f }, { 1.f, 0.f, 0.f, 3.f }, { .5f + .5f / 3.f, 1.f } },
		{ {-bolen, 3.f * bolen, 0.f }, { 0.f, 0.f, 1.f }, { 1.f, 0.f, 0.f, 3.f }, { .5f - .5f / 3.f, 1.f } },

		// south sleeve
		{ { bolen, -1.f * bolen, 0.f }, { 0.f, 0.f, 1.f }, { 1.f, 0.f, 0.f, 0.f }, { .5f + .5f / 3.f, .5f - .5f / 3.f } },
		{ {-bolen, -1.f * bolen, 0.f }, { 0.f, 0.f, 1.f }, { 1.f, 0.f, 0.f, 0.f }, { .5f - .5f / 3.f, .5f - .5f / 3.f } },
		{ { bolen, -2.f * bolen, 0.f }, { 0.f, 0.f, 1.f }, { 1.f, 0.f, 0.f, 8.f }, { .5f + .5f / 3.f, .5f - 1.f / 3.f } },
		{ {-bolen, -2.f * bolen, 0.f }, { 0.f, 0.f, 1.f }, { 1.f, 0.f, 0.f, 8.f }, { .5f - .5f / 3.f, .5f - 1.f / 3.f } },
		{ { bolen, -3.f * bolen, 0.f }, { 0.f, 0.f, 1.f }, { 1.f, 0.f, 0.f, 9.f }, { .5f + .5f / 3.f, 0.f } },
		{ {-bolen, -3.f * bolen, 0.f }, { 0.f, 0.f, 1.f }, { 1.f, 0.f, 0.f, 9.f }, { .5f - .5f / 3.f, 0.f } },

		// east sleeve
		{ { 2.f * bolen,  bolen, 0.f }, { 0.f, 0.f, 1.f }, { 1.f, 0.f, 0.f, 11.f }, { .5f + 1.f / 3.f, .5f + .5f / 3.f } },
		{ { 2.f * bolen, -bolen, 0.f }, { 0.f, 0.f, 1.f }, { 1.f, 0.f, 0.f, 11.f }, { .5f + 1.f / 3.f, .5f - .5f / 3.f } },
		{ { 3.f * bolen,  bolen, 0.f }, { 0.f, 0.f, 1.f }, { 1.f, 0.f, 0.f, 12.f }, { 1.f,             .5f + .5f / 3.f } },
		{ { 3.f * bolen, -bolen, 0.f }, { 0.f, 0.f, 1.f }, { 1.f, 0.f, 0.f, 12.f }, { 1.f,             .5f - .5f / 3.f } },

		// west sleeve
		{ {-2.f * bolen,  bolen, 0.f }, { 0.f, 0.f, 1.f }, { 1.f, 0.f, 0.f, 5.f }, { .5f - 1.f / 3.f, .5f + .5f / 3.f } },
		{ {-2.f * bolen, -bolen, 0.f }, { 0.f, 0.f, 1.f }, { 1.f, 0.f, 0.f, 5.f }, { .5f - 1.f / 3.f, .5f - .5f / 3.f } },
		{ {-3.f * bolen,  bolen, 0.f }, { 0.f, 0.f, 1.f }, { 1.f, 0.f, 0.f, 6.f }, { 0.f,             .5f + .5f / 3.f } },
		{ {-3.f * bolen, -bolen, 0.f }, { 0.f, 0.f, 1.f }, { 1.f, 0.f, 0.f, 6.f }, { 0.f,             .5f - .5f / 3.f } }
	};

	static const uint16_t idx[][3] =
	{
		{  0,  1,  2 },
		{  1,  3,  2 },
		{  2,  3,  4 },
		{  3,  5,  4 },
		{  4,  5,  6 },

		{  0,  8,  7 },
		{  7,  8,  9 },
		{  9,  8, 10 },
		{  9, 10, 11 },
		{ 11, 10, 12 },

		{  0,  7,  1 },
		{  7, 14,  1 },
		{  1, 14, 13 },
		{ 14, 16, 13 },
		{ 13, 16, 15 },

		{  0,  2,  8 },
		{  8,  2, 18 },
		{ 18,  2, 17 },
		{ 18, 17, 20 },
		{ 20, 17, 19 }
	};

	g_num_faces[MESH_SKIN] = sizeof(idx) / sizeof(idx[0]);

	const unsigned num_verts = sizeof(arr) / sizeof(arr[0]);
	const unsigned num_indes = sizeof(idx) / sizeof(idx[0][0]);

	stream::cout << "number of vertices: " << num_verts <<
		"\nnumber of indices: " << num_indes << '\n';

	/////////////////////////////////////////////////////////////////
	// reserve VAO (if available) and all necessary VBOs

#if PLATFORM_GL_OES_vertex_array_object
	glGenVertexArraysOES(sizeof(g_vao) / sizeof(g_vao[0]), g_vao);

	for (unsigned i = 0; i < sizeof(g_vao) / sizeof(g_vao[0]); ++i)
		assert(g_vao[i]);

	glBindVertexArrayOES(g_vao[PROG_SKIN]);

#endif
	glGenBuffers(sizeof(g_vbo) / sizeof(g_vbo[0]), g_vbo);

	for (unsigned i = 0; i < sizeof(g_vbo) / sizeof(g_vbo[0]); ++i)
		assert(g_vbo[i]);

	/////////////////////////////////////////////////////////////////
	// bind VBOs and upload the geom asset

	glBindBuffer(GL_ARRAY_BUFFER, g_vbo[VBO_SKIN_VTX]);
	glBufferData(GL_ARRAY_BUFFER, sizeof(arr), arr, GL_STATIC_DRAW);

	DEBUG_GL_ERR()

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, g_vbo[VBO_SKIN_IDX]);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(idx), idx, GL_STATIC_DRAW);

	DEBUG_GL_ERR()

	/////////////////////////////////////////////////////////////////
	// set up the vertex attrib mapping for all attrib inputs

	if (!setupVertexAttrPointers< Vertex >(g_active_attr_semantics[PROG_SKIN])) {
		stream::cerr << __FUNCTION__ << " failed at setupVertexAttrPointers\n";
		return false;
	}

#if PLATFORM_GL_OES_vertex_array_object
	/////////////////////////////////////////////////////////////////
	// if VAO: the final step of enabling the mapped attrib inputs

	for (unsigned i = 0; i < g_active_attr_semantics[PROG_SKIN].num_active_attr; ++i)
		glEnableVertexAttribArray(g_active_attr_semantics[PROG_SKIN].active_attr[i]);

	DEBUG_GL_ERR()

	glBindVertexArrayOES(0);

#endif
	/////////////////////////////////////////////////////////////////
	// unbind the VBOs -- will be re-bound on a need-to-use basis

	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

	on_error.reset();
	return true;
}

} // namespace

namespace { // anonymous

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

namespace hook {

bool render_frame(GLuint /* primary_fbo */)
{
	if (!check_context(__FUNCTION__))
		return false;

	/////////////////////////////////////////////////////////////////
	// clear the framebuffer (both color and depth)

	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	/////////////////////////////////////////////////////////////////
	// query about the viewport geometry; used for aspect

	GLint vp[4];
	glGetIntegerv(GL_VIEWPORT, vp);

	/////////////////////////////////////////////////////////////////
	// animate the skeleton for the given timestamp

	static float anim = 0.f;

	rend::animateSkeleton(bone_count, g_bone_mat, g_bone, g_skeletal_animation, anim);

	anim += g_anim_step;

	if (1.f < anim) {
		anim -= floorf(anim);
		rend::resetSkeletonAnimProgress(g_skeletal_animation);
	}

	/////////////////////////////////////////////////////////////////
	// produce mvp matrix

	const simd::matx4 mv = simd::matx4(
		1.f,  0.f,  0.f,  0.f,
		0.f,  1.f,  0.f,  0.f,
		0.f,  0.f,  1.f,  0.f,
		0.f,  0.f, -2.f,  1.f);

	const float aspect = float(vp[2]) / vp[3];
	const float l = aspect * -.5f;
	const float r = aspect * .5f;
	const float b = -.5f;
	const float t = .5f;
	const float n = 1.f;
	const float f = 4.f;

	const matx4_persp proj(l, r, b, t, n, f);

	const simd::matx4 mvp = simd::matx4().mul(mv, proj);
	const rend::dense_matx4 dense_mvp = rend::dense_matx4(
			mvp[0][0], mvp[0][1], mvp[0][2], mvp[0][3],
			mvp[1][0], mvp[1][1], mvp[1][2], mvp[1][3],
			mvp[2][0], mvp[2][1], mvp[2][2], mvp[2][3],
			mvp[3][0], mvp[3][1], mvp[3][2], mvp[3][3]);

	/////////////////////////////////////////////////////////////////
	// activate the shader program and set up all valid uniform vars

	glUseProgram(g_shader_prog[PROG_SKIN]);

	DEBUG_GL_ERR()

	if (-1 != g_uni[PROG_SKIN][UNI_MVP]) {
		glUniformMatrix4fv(g_uni[PROG_SKIN][UNI_MVP],
			1, GL_FALSE, static_cast< const GLfloat* >(dense_mvp));

		DEBUG_GL_ERR()
	}

	if (-1 != g_uni[PROG_SKIN][UNI_BONE]) {
		glUniformMatrix4fv(g_uni[PROG_SKIN][UNI_BONE],
			bone_count, GL_FALSE, static_cast< const GLfloat* >(g_bone_mat[0]));

		DEBUG_GL_ERR()
	}

	if (-1 != g_uni[PROG_SKIN][UNI_LP_OBJ]) {
		const GLfloat nonlocal_light[4] = {
			mv[0][2],
			mv[1][2],
			mv[2][2],
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

#if PLATFORM_GL_OES_vertex_array_object
	glBindVertexArrayOES(g_vao[PROG_SKIN]);

	DEBUG_GL_ERR()

#else
	/////////////////////////////////////////////////////////////////
	// no VAO: re-bind the VBOs and enable all mapped vertex attribs

	glBindBuffer(GL_ARRAY_BUFFER, g_vbo[VBO_SKIN_VTX]);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, g_vbo[VBO_SKIN_IDX]);

	for (unsigned i = 0; i < g_active_attr_semantics[PROG_SKIN].num_active_attr; ++i)
		glEnableVertexAttribArray(g_active_attr_semantics[PROG_SKIN].active_attr[i]);

	DEBUG_GL_ERR()

#endif
	/////////////////////////////////////////////////////////////////
	// draw the geometry asset

	glDrawElements(GL_TRIANGLES, g_num_faces[MESH_SKIN] * 3, GL_UNSIGNED_SHORT, (void*) 0);

	DEBUG_GL_ERR()

#if PLATFORM_GL_OES_vertex_array_object == 0
	/////////////////////////////////////////////////////////////////
	// no VAO: disable all mapped vertex attribs

	for (unsigned i = 0; i < g_active_attr_semantics[PROG_SKIN].num_active_attr; ++i)
		glDisableVertexAttribArray(g_active_attr_semantics[PROG_SKIN].active_attr[i]);

	DEBUG_GL_ERR()

#endif
	return true;
}

} // namespace
