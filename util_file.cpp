#include <sys/types.h>
#include <sys/stat.h>
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

#include "scoped.hpp"
#include "util_file.hpp"

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

template <>
class scoped_functor< FILE >
{
public:
	void operator()(FILE* arg)
	{
		fclose(arg);
	}
};

bool get_file_size(
	const char* const filename,
	size_t& size)
{
	assert(0 != filename);
	struct stat filestat;

	if (-1 == stat(filename, &filestat)) {
		fprintf(stderr, "%s cannot stat file '%s'\n", __FUNCTION__, filename);
		return false;
	}

	if (!S_ISREG(filestat.st_mode)) {
		fprintf(stderr, "%s encountered a non-regular file '%s'\n", __FUNCTION__, filename);
		return false;
	}

	size = filestat.st_size;
	return true;
}

char* get_buffer_from_file(
	const char* const filename,
	size_t& size,
	const size_t roundToIntegralMultiple)
{
	assert(0 != filename);
	assert(0 == (roundToIntegralMultiple & roundToIntegralMultiple - 1));

	if (!get_file_size(filename, size)) {
		fprintf(stderr, "%s cannot get size of file '%s'\n", __FUNCTION__, filename);
		return 0;
	}

	const scoped_ptr< FILE, scoped_functor > file(fopen(filename, "rb"));

	if (0 == file()) {
		fprintf(stderr, "%s cannot open file '%s'\n", __FUNCTION__, filename);
		return 0;
	}

	const size_t roundTo = roundToIntegralMultiple - 1;
	scoped_ptr< char, generic_free > source(
		reinterpret_cast< char* >(malloc((size + roundTo) & ~roundTo)));

	if (0 == source()) {
		fprintf(stderr, "%s cannot allocate memory for file '%s'\n", __FUNCTION__, filename);
		return 0;
	}

	if (1 != fread(source(), size, 1, file())) {
		fprintf(stderr, "%s cannot read from file '%s'\n", __FUNCTION__, filename);
		return 0;
	}

	char* const ret = source();
	source.reset();

	return ret;
}

} // namespace util
