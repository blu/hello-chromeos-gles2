#ifndef util_misc_H__
#define util_misc_H__

#include <stdio.h>
#if PLATFORM_GL
	#include <GL/gl.h>
#else
	#include <GLES2/gl2.h>
#endif

#include <string>

namespace util {

template < bool >
struct compile_assert;

template <>
struct compile_assert< true > {
	compile_assert() {
	}
};

bool setupShader(
	const GLuint shader_name,
	const char* const filename);

bool setupShaderWithPatch(
	const GLuint shader_name,
	const char* const filename,
	const size_t patch_count,
	const std::string* const patch);

bool setupProgram(
	const GLuint prog,
	const GLuint shader_vert,
	const GLuint shader_frag);

bool reportGLError(FILE* file = stderr);
bool reportEGLError(FILE* file = stderr);

} // namespace util

namespace hook {

bool init_resources(
	const unsigned argc,
	const char* const* argv);

bool deinit_resources();
bool requires_depth();
bool render_frame(GLuint prime_fbo);
bool set_num_drawcalls(const unsigned);
unsigned get_num_drawcalls();

} // namespace hook

#endif // util_misc_H__
