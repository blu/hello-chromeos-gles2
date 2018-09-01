#if PLATFORM_GL
	#include <GL/gl.h>
#else
	#include <GLES2/gl2.h>
#endif

#include <assert.h>
#include <stdlib.h>
#include <cstring>
#include <string>

#include "util_file.hpp"
#include "util_misc.hpp"
#include "scoped.hpp"
#include "pure_macro.hpp"
#include "stream.hpp"

using util::scoped_ptr;

template < typename T >
class generic_free
{
public:
	void operator()(T* arg)
	{
		free(arg);
	}
};

static bool setupShaderFromString(
	const GLuint shader_name,
	const char* const source,
	const size_t length)
{
	assert(0 != source);
	assert(0 != length);

	if (GL_FALSE == glIsShader(shader_name)) {
		stream::cerr << __FUNCTION__ << " argument is not a valid shader object\n";
		return false;
	}

#if PLATFORM_GLES
	GLboolean compiler_present = GL_FALSE;
	glGetBooleanv(GL_SHADER_COMPILER, &compiler_present);

	if (GL_TRUE != compiler_present) {
		stream::cerr << "no shader compiler present (binary only)\n";
		return false;
	}

	const char* const ver_marker = "///essl ";

#else
	const char* const ver_marker = "///glsl ";

#endif
	// isolate an optional version comment for this platform
	const char* const ver_pragma = "#version ";
	const size_t marker_len = strlen(ver_marker);
	const size_t pragma_len = strlen(ver_pragma);

	GLint srclen[] = {
		GLint(length),
		GLint(length)
	};
	const char* src[] = {
		source,
		source
	};
	GLsizei src_count = 1;

	if (marker_len + pragma_len < length) {
		const size_t seek_len = length - marker_len - pragma_len;
		size_t seek_pos = 0;
		while (seek_pos < seek_len) {
			// seek a version marker comment..
			if (0 == strncmp(source + seek_pos, ver_marker, marker_len)) {
				seek_pos += marker_len;
				// followed by a version pragma
				if (0 == strncmp(source + seek_pos, ver_pragma, pragma_len)) {
					src[0] = source + seek_pos;
					// seek a line terminatior
					while (seek_pos < length) {
						if (source[seek_pos] == '\n')
							break;
						++seek_pos;
					}
					srclen[0] = source + seek_pos - src[0];
					++src_count;
				}
				break;
			}
			++seek_pos;
		}
	}

	glShaderSource(shader_name, src_count, src, srclen);
	glCompileShader(shader_name);

	if (util::reportGLError()) {
		stream::cerr << "cannot compile shader from source (binary only?)\n";
		return false;
	}

	GLint success = GL_FALSE;
	glGetShaderiv(shader_name, GL_COMPILE_STATUS, &success);

	if (GL_TRUE != success) {
		GLint len;
		glGetShaderiv(shader_name, GL_INFO_LOG_LENGTH, &len);
		const scoped_ptr< GLchar, generic_free > log(
			reinterpret_cast< GLchar* >(malloc(sizeof(GLchar) * len)));
		glGetShaderInfoLog(shader_name, len, NULL, log());
		stream::cerr << "shader compile log:\n";
		stream::cerr.write(log(), len - 1);
		stream::cerr << '\n';
		return false;
	}

	return true;
}

bool util::setupShader(
	const GLuint shader_name,
	const char* const filename)
{
	assert(0 != filename);

	size_t length;
	const scoped_ptr< char, generic_free > source(get_buffer_from_file(filename, length));

	if (0 == source()) {
		stream::cerr << __FUNCTION__ << " failed to read shader file '" << filename << "'\n";
		return false;
	}

	return setupShaderFromString(shader_name, source(), length);
}

bool util::setupShaderWithPatch(
	const GLuint shader_name,
	const char* const filename,
	const size_t patch_count,
	const std::string* const patch)
{
	assert(0 != filename);
	assert(0 != patch);

	size_t length;
	const scoped_ptr< char, generic_free > source(get_buffer_from_file(filename, length));

	if (0 == source()) {
		stream::cerr << __FUNCTION__ << " failed to read shader file '" << filename << "'\n";
		return false;
	}

	std::string src_final(source(), length);
	size_t npatched = 0;

	for (size_t i = 0; i < patch_count; ++i) {
		const std::string& patch_src = patch[i * 2 + 0];
		const std::string& patch_dst = patch[i * 2 + 1];

		if (patch_src.empty() || patch_src == patch_dst)
			continue;

		const size_t len_src = patch_src.length();
		const size_t len_dst = patch_dst.length();

		for (size_t pos = src_final.find(patch_src);
			std::string::npos != pos;
			pos = src_final.find(patch_src, pos + len_dst)) {

			src_final.replace(pos, len_src, patch_dst);
			++npatched;

			stream::cout << "turn: " << patch_src << "\ninto: " << patch_dst << '\n';
		}
	}

	if (npatched)
		stream::cout << "substitutions: " << npatched << '\n';

	return setupShaderFromString(shader_name, src_final.c_str(), src_final.length());
}

bool util::setupProgram(
	const GLuint prog,
	const GLuint shader_vert,
	const GLuint shader_frag)
{
	if (GL_FALSE == glIsProgram(prog)) {
		stream::cerr << __FUNCTION__ << " argument is not a valid program object\n";
		return false;
	}

	if (GL_FALSE == glIsShader(shader_vert) ||
		GL_FALSE == glIsShader(shader_frag)) {

		stream::cerr << __FUNCTION__ << " argument is not a valid shader object\n";
		return false;
	}

	glAttachShader(prog, shader_vert);
	glAttachShader(prog, shader_frag);
	glLinkProgram(prog);

	GLint success = GL_FALSE;
	glGetProgramiv(prog, GL_LINK_STATUS, &success);

	if (GL_TRUE != success) {
		GLint len;
		glGetProgramiv(prog, GL_INFO_LOG_LENGTH, &len);
		const scoped_ptr< GLchar, generic_free > log(
			reinterpret_cast< GLchar* >(malloc(sizeof(GLchar) * len)));
		glGetProgramInfoLog(prog,  len,  NULL, log());
		stream::cerr << "shader link log:\n";
		stream::cerr.write(log(), len - 1);
		stream::cerr << '\n';
		return false;
	}

	return true;
}

