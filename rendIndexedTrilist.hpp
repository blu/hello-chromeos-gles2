#ifndef rend_indexed_trilist_H__
#define rend_indexed_trilist_H__

#if defined(PLATFORM_GL)
	#include <GL/gl.h>
#else
	#include <GLES2/gl2.h>
#endif

namespace util {

bool
fill_indexed_trilist_from_file_ABE(
	const char* const filename,
	const GLuint vbo_arr,
	const GLuint vbo_idx,
	const uintptr_t (&semantics_offset)[4],
	unsigned& num_faces,
	GLenum& index_type,
	float (&bmin)[3],
	float (&bmax)[3]);

bool
fill_indexed_trilist_from_file_Ogre(
	const char* const filename,
	const GLuint vbo_arr,
	const GLuint vbo_idx,
	const uintptr_t (&semantics_offset)[4],
	unsigned& num_faces,
	GLenum& index_type,
	float (&bmin)[3],
	float (&bmax)[3]);

} // namespace util

#endif // rend_indexed_trilist_H__
