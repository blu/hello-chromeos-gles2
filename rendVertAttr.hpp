#ifndef rend_vert_attr_H__
#define rend_vert_attr_H__

#ifdef PLATFORM_GL
	#include <GL/gl.h>
#else
	#include <GLES2/gl2.h>
#endif

#include <stdint.h>
#include <cstddef>

namespace rend
{

struct ActiveAttrSemantics
{
	GLint active_attr[32];
	unsigned num_active_attr;

	int semantics_vertex;
	int semantics_normal;
	int semantics_tangent;
	int semantics_blendw;
	int semantics_tcoord;
	int semantics_index;

	ActiveAttrSemantics()
	: num_active_attr(0)
	, semantics_vertex(-1)
	, semantics_normal(-1)
	, semantics_tangent(-1)
	, semantics_blendw(-1)
	, semantics_tcoord(-1)
	, semantics_index(-1)
	{}

	int registerAttr(
		const GLint attr);

	bool registerVertexAttr(
		const GLint attr);

	bool registerNormalAttr(
		const GLint attr);

	bool registerTangentAttr(
		const GLint attr);

	bool registerBlendWAttr(
		const GLint attr);

	bool registerTCoordAttr(
		const GLint attr);

	bool registerIndexAttr(
		const GLint attr);

	GLint getVertexAttr() const;
	GLint getNormalAttr() const;
	GLint getTangentAttr() const;
	GLint getBlendWAttr() const;
	GLint getTCoordAttr() const;
	GLint getIndexAttr() const;
};


inline int
ActiveAttrSemantics::registerAttr(
	const GLint attr)
{
	if (attr != -1) {
		assert(num_active_attr < sizeof(active_attr) / sizeof(active_attr[0]));

		const int n = num_active_attr;
		active_attr[num_active_attr++] = attr;
		return n;
	}

	return -1;
}


inline bool
ActiveAttrSemantics::registerVertexAttr(
	const GLint attr)
{
	assert(-1 == semantics_vertex);

	if (-1 == semantics_vertex)
		return -1 != (semantics_vertex = registerAttr(attr));

	return false;
}


inline bool
ActiveAttrSemantics::registerNormalAttr(
	const GLint attr)
{
	assert(-1 == semantics_normal);

	if (-1 == semantics_normal)
		return -1 != (semantics_normal = registerAttr(attr));

	return false;
}


inline bool
ActiveAttrSemantics::registerTangentAttr(
	const GLint attr)
{
	assert(-1 == semantics_tangent);

	if (-1 == semantics_tangent)
		return -1 != (semantics_tangent = registerAttr(attr));

	return false;
}


inline bool
ActiveAttrSemantics::registerBlendWAttr(
	const GLint attr)
{
	assert(-1 == semantics_blendw);

	if (-1 == semantics_blendw)
		return -1 != (semantics_blendw = registerAttr(attr));

	return false;
}


inline bool
ActiveAttrSemantics::registerTCoordAttr(
	const GLint attr)
{
	assert(-1 == semantics_tcoord);

	if (-1 == semantics_tcoord)
		return -1 != (semantics_tcoord = registerAttr(attr));

	return false;
}


inline bool
ActiveAttrSemantics::registerIndexAttr(
	const GLint attr)
{
	assert(-1 == semantics_index);

	if (-1 == semantics_index)
		return -1 != (semantics_index = registerAttr(attr));

	return false;
}


inline GLint
ActiveAttrSemantics::getVertexAttr() const
{
	assert(unsigned(semantics_vertex) < num_active_attr);

	if (unsigned(semantics_vertex) < num_active_attr)
		return active_attr[semantics_vertex];

	return -1;
}


inline GLint
ActiveAttrSemantics::getNormalAttr() const
{
	assert(unsigned(semantics_normal) < num_active_attr);

	if (unsigned(semantics_normal) < num_active_attr)
		return active_attr[semantics_normal];

	return -1;
}


inline GLint
ActiveAttrSemantics::getTangentAttr() const
{
	assert(unsigned(semantics_tangent) < num_active_attr);

	if (unsigned(semantics_tangent) < num_active_attr)
		return active_attr[semantics_tangent];

	return -1;
}


inline GLint
ActiveAttrSemantics::getBlendWAttr() const
{
	assert(unsigned(semantics_blendw) < num_active_attr);

	if (unsigned(semantics_blendw) < num_active_attr)
		return active_attr[semantics_blendw];

	return -1;
}


inline GLint
ActiveAttrSemantics::getTCoordAttr() const
{
	assert(unsigned(semantics_tcoord) < num_active_attr);

	if (unsigned(semantics_tcoord) < num_active_attr)
		return active_attr[semantics_tcoord];

	return -1;
}


inline GLint
ActiveAttrSemantics::getIndexAttr() const
{
	assert(unsigned(semantics_index) < num_active_attr);

	if (unsigned(semantics_index) < num_active_attr)
		return active_attr[semantics_index];

	return -1;
}

} // namespace rend

#endif // rend_vert_attr_H__
