#ifndef util_tex_H__
#define util_tex_H__

#include <stdint.h>
#if PLATFORM_GL
	#include <GL/gl.h>
#else
	#include <GLES2/gl2.h>
#endif

namespace util {

struct pix
{
	uint8_t c[3];

	pix() {}
	pix(const uint8_t r,
		const uint8_t g,
		const uint8_t b)
	{
		c[0] = r;
		c[1] = g;
		c[2] = b;
	}
};

bool setupTexture2D(
	const GLuint tex_name,
	const pix* const buffer,
	const unsigned tex_w,
	const unsigned tex_h,
	const bool sampleNearest = false);

bool setupTexture2D(
	const GLuint tex_name,
	const char* const filename,
	unsigned& tex_w,
	unsigned& tex_h,
	const bool sampleNearest = false);

} // namespace util

#endif // util_tex_H__
