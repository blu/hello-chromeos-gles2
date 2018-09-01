#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#include "scoped.hpp"
#include "util_file.hpp"
#include "util_tex.hpp"
#include "util_misc.hpp"

namespace util {

template < typename T >
class generic_free
{
public:
	void operator()(T* arg)
	{
		free(arg);
	}
};

static size_t integral_size(
	const size_t size)
{
	if (0 == (size & (size - 1)))
		return size;

	// compute optimal integral size in two steps
	size_t integralSize = size;

	// first, isolate the leading bit in the natural size
	while (integralSize & integralSize - 1)
		integralSize &= integralSize - 1;

	// second, double up as the natural size is not power of two
	return integralSize * 2;
}

static size_t next_multiple_of_pix_integral(
	const size_t unaligned_size)
{
	const size_t integralSize = integral_size(sizeof(pix)) - 1;

	// round up the unaligned size to a multiple of pix's integral size
	return (unaligned_size + integralSize) & ~integralSize;
}

static bool fill_from_file(
	const pix* const buffer,
	unsigned& tex_w,
	unsigned& tex_h,
	const size_t fileSize)
{
	if (0 == buffer)
		return false;

	const uint32_t* header = reinterpret_cast< const uint32_t* >(buffer);

	const uint32_t w = header[0];
	const uint32_t h = header[1];

	if (size_t(w) * size_t(h) * sizeof(pix) == fileSize - sizeof(uint32_t[2])) {
		tex_w = w;
		tex_h = h;
		return true;
	}

	return false;
}

static void fill_with_checker(
	pix* const buffer,
	const unsigned stride,
	const unsigned dim_x,
	const unsigned dim_y,
	const unsigned granularity = 8)
{
	const pix pixA(255U, 128U, 128U);
	const pix pixB(128U, 128U, 255U);

	for (unsigned y = 0; y < dim_y; ++y)
		for (unsigned x = 0; x < dim_x; ++x) {
			*(reinterpret_cast< pix* >(reinterpret_cast< uint8_t* >(buffer) + y * stride) + x) =
				(y & granularity) ^ (x & granularity) ? pixB : pixA;
		}
}

bool setupTexture2D(
	const GLuint tex_name,
	const pix* const buffer,
	const unsigned tex_w,
	const unsigned tex_h,
	const bool sampleNearest)
{
	assert(0 != tex_name);
	assert(0 != buffer);
	assert(0 != tex_w && 0 != tex_h);

	const size_t pix_size = sizeof(pix);
	const size_t tex_size = tex_h * tex_w * pix_size;

	fprintf(stdout, "%u x %u x %u bpp, %u bytes\n", tex_w, tex_h, unsigned(pix_size * 8), unsigned(tex_size));

	const bool pot = 0 == (tex_w & tex_w - 1) && 0 == (tex_h & tex_h - 1);

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, tex_name);

	if (sampleNearest) {
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, pot ? GL_NEAREST_MIPMAP_NEAREST : GL_NEAREST);
	}
	else {
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, pot ? GL_LINEAR_MIPMAP_NEAREST : GL_LINEAR);
	}
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, tex_w, tex_h, 0, GL_RGB, GL_UNSIGNED_BYTE, 0);
	glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, tex_w, tex_h, GL_RGB, GL_UNSIGNED_BYTE, buffer);

	if (pot) {
		glGenerateMipmap(GL_TEXTURE_2D);
	}

	glBindTexture(GL_TEXTURE_2D, 0);

	const bool success = !reportGLError(stderr);
	return success;
}

bool setupTexture2D(
	const GLuint tex_name,
	const char* const filename,
	unsigned& tex_w,
	unsigned& tex_h,
	const bool sampleNearest)
{
	assert(0 != tex_name);
	assert(0 != filename);
	assert(0 != tex_w && 0 != tex_h);

	const size_t pix_size = sizeof(pix);
	const pix* start = 0;
	size_t fileSize;

	// provide some guardband as pixels are of non-word-multiple size
	scoped_ptr< pix, generic_free > tex_src(
		reinterpret_cast< pix* >(get_buffer_from_file(filename, fileSize, integral_size(sizeof(pix)))));

	if (0 != tex_src() && fill_from_file(tex_src(), tex_w, tex_h, fileSize)) {
		fprintf(stdout, "texture bitmap '%s' ", filename);
		const size_t header_size = sizeof(uint32_t[2]);

		start = reinterpret_cast< pix*>(
			reinterpret_cast< uint8_t* >(tex_src()) + header_size);
	}
	else { // default to checker texture
		const size_t tex_size = tex_w * tex_h * pix_size;

		// provide some guardband as pixels are of non-word-multiple size
		scoped_ptr< pix, generic_free > chk_src(
			reinterpret_cast< pix* >(malloc(next_multiple_of_pix_integral(tex_size))));

		if (0 == chk_src()) {
			fprintf(stderr, "%s failed to allocate texture checker\n", __FUNCTION__);
			return false;
		}

		fill_with_checker(chk_src(), tex_w * pix_size, tex_w, tex_h);
		fprintf(stdout, "texture checker ");

		start = chk_src();
		tex_src.swap(chk_src);
	}

	return setupTexture2D(tex_name, start, tex_w, tex_h, sampleNearest);
}

} // namespace util
