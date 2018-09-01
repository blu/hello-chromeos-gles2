////////////////////////////////////////////////////////////////////////////////
// simple png-to-raw converter
//
// build as: $ g++ raw_from_png.cpp -lpng16

#include <iostream>
#include <stdint.h>
#include <stdio.h>
#include <png.h>

struct PngUserState
{
	FILE *fp;
	png_structp png_ptr;
	png_infop info_ptr;
	png_infop end_info;

	PngUserState()
	: fp(0)
	, png_ptr(0)
	, info_ptr(0)
	, end_info(0)
	{}

	~PngUserState()
	{
		if (0 != png_ptr) {
			const png_infopp info_pptr = 0 != info_ptr ? &info_ptr : (png_infopp) 0;
			const png_infopp endfo_pptr = 0 != end_info ? &end_info : (png_infopp) 0;

			png_destroy_read_struct(&png_ptr, info_pptr, endfo_pptr);
		}

		if (0 != fp)
			fclose(fp);
	}
};


static void
png_error_fn(
	png_structp p,
	png_const_charp)
{
	std::cerr << "critical libpng issue, cleaning up user data" << std::endl;

	PngUserState* pus = reinterpret_cast< PngUserState* >(png_get_error_ptr(p));

	pus->~PngUserState();
}


static void
png_warn_fn(
	png_structp,
	png_const_charp)
{
	std::cerr << "non-critical libpng issue, carrying on" << std::endl;
}


int
main(
	int argc,
	char** argv)
{
	if (2 > argc || 3 < argc) {
		std::cerr << "usage: " << argv[0] << " png_file [raw_file]" << std::endl;
		return -1;
	}

	const char* const png_name = argv[1];
	const char* const raw_name = 3 == argc ? argv[2] : "out.raw";

	PngUserState pus;

	pus.fp = fopen(png_name, "rb");

	if (0 == pus.fp) {
		std::cerr << "failure at opening input file '" << png_name << "'" << std::endl;
		return -1;
	}

	uint8_t header[8];
	const size_t sizeof_header = sizeof(header);

	if (1 != fread(header, sizeof_header, 1, pus.fp)) {
		std::cerr << "failure at reading png header" << std::endl;
		return -1;
	}

	if (png_sig_cmp(header, 0, sizeof_header)) {
		std::cerr << "failure at recognizing png file" << std::endl;
		return -1;
	}

	pus.png_ptr = png_create_read_struct(
		PNG_LIBPNG_VER_STRING,
		(png_voidp) &pus,
		png_error_fn,
		png_warn_fn);

	if (0 == pus.png_ptr) {
		std::cerr << "failure at png_create_read_struct()" << std::endl;
		return -1;
	}

	pus.info_ptr = png_create_info_struct(pus.png_ptr);

	if (0 == pus.info_ptr) {
		std::cerr << "failure at png_create_info_struct()" << std::endl;
		return -1;
	}

	pus.end_info = png_create_info_struct(pus.png_ptr);

	if (0 == pus.end_info) {
		std::cerr << "failure at png_create_info_struct" << std::endl;
		return -1;
	}

#if 0 // prefer other error-handling methods to setjmp
	if (setjmp(png_jmpbuf(pus.png_ptr))) {
		std::cerr << "critical libpng issue, bailing out" << std::endl;
		return -1;
	}
#endif

	png_init_io(pus.png_ptr, pus.fp);
	png_set_sig_bytes(pus.png_ptr, sizeof_header);

	png_read_png(
		pus.png_ptr,
		pus.info_ptr,
		PNG_TRANSFORM_STRIP_16 |
		PNG_TRANSFORM_STRIP_ALPHA |
		PNG_TRANSFORM_PACKING |
		PNG_TRANSFORM_GRAY_TO_RGB,
		0);

	fclose(pus.fp);

	pus.fp = fopen(raw_name, "wb");

	if (0 == pus.fp) {
		std::cerr << "failure at opening output file" << std::endl;
		return -1;
	}

	const png_uint_32 image_w = png_get_image_width(pus.png_ptr, pus.info_ptr);
	const png_uint_32 image_h = png_get_image_height(pus.png_ptr, pus.info_ptr);

	std::cout << image_w << 'x' << image_h << std::endl;

	const uint32_t image_dim[] = { image_w, image_h };

	if (1 != fwrite(image_dim, sizeof(image_dim), 1, pus.fp)) {
		std::cerr << "failure at writing image dims to output" << std::endl;
		return -1;
	}

	png_bytep* const row_pointers = png_get_rows(pus.png_ptr, pus.info_ptr);

	for (unsigned i = 0; i < image_h; ++i) {
		const unsigned sizeofpix = sizeof(uint8_t[3]);

		if (1 != fwrite(row_pointers[image_h - 1 - i], image_w * sizeofpix, 1, pus.fp)) {
			std::cerr << "failure at writing image row to output" << std::endl;
			return -1;
		}
	}

	return 0;
}
