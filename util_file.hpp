#ifndef util_file_H__
#define util_file_H__

namespace util {

bool get_file_size(
	const char* const filename,
	size_t& size);

char* get_buffer_from_file(
	const char* const filename,
	size_t& size,
	const size_t roundToIntegralMultiple = 1);

} // namespace util

#endif // util_file_H__
