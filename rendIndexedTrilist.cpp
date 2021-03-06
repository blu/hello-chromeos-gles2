#if defined(PLATFORM_GL)
	#include <GL/gl.h>
	#include "gles_gl_mapping.hpp"
#else
	#include <GLES2/gl2.h>
	#include <GLES2/gl2ext.h>
	#include "gles_ext.h"
#endif

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <limits>
#include <iomanip>
#include <vector>

#include "scoped.hpp"
#include "stream.hpp"
#include "rendIndexedTrilist.hpp"

#ifndef MESH_INDEX_START
#define MESH_INDEX_START 0
#endif

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

template < typename T, unsigned NUM_T >
int fscanf_generic(
	FILE*,
	T (&)[NUM_T])
{
	assert(false);
	return 0;
}

template <>
int fscanf_generic(
	FILE* file,
	uint16_t (&out)[3])
{
	return fscanf(file, "%hu %hu %hu",
		&out[0],
		&out[1],
		&out[2]);
}

template <>
int fscanf_generic(
	FILE* file,
	uint32_t (&out)[3])
{
	return fscanf(file, "%u %u %u",
		&out[0],
		&out[1],
		&out[2]);
}

template <>
int fscanf_generic(
	FILE* file,
	float (&out)[3])
{
	return fscanf(file, "%f %f %f",
		&out[0],
		&out[1],
		&out[2]);
}

template <>
int fscanf_generic(
	FILE* file,
	float (&out)[6])
{
	return fscanf(file, "%f %f %f %f %f %f",
		&out[0],
		&out[1],
		&out[2],
		&out[3],
		&out[4],
		&out[5]);
}

template <>
int fscanf_generic(
	FILE* file,
	float (&out)[8])
{
	return fscanf(file, "%f %f %f %f %f %f %f %f",
		&out[0],
		&out[1],
		&out[2],
		&out[3],
		&out[4],
		&out[5],
		&out[6],
		&out[7]);
}

template < typename T >
const T& min(
	const T& a,
	const T& b)
{
	return a < b ? a : b;
}

template < typename T >
const T& max(
	const T& a,
	const T& b)
{
	return a > b ? a : b;
}

template <
	unsigned NUM_FLOATS_T,		// floats per vertex
	unsigned NUM_INDICES_T >	// indices per face
bool
fill_indexed_facelist_from_file(
	const char* const filename,
	const GLuint vbo_arr,
	const GLuint vbo_idx,
	unsigned& num_faces,
	GLenum& index_type,
	float (&vmin)[3],
	float (&vmax)[3])
{
	assert(filename);

	scoped_ptr< FILE, scoped_functor > file(fopen(filename, "r"));

	if (0 == file())
	{
		stream::cerr << __FUNCTION__ << " failed at fopen '" << filename << "'\n";
		return false;
	}

	unsigned nv_total = 0;
	unsigned nf_total = 0;
	void *vb_total = 0;
	void *ib_total = 0;

	vmin[0] = std::numeric_limits< float >::infinity();
	vmin[1] = std::numeric_limits< float >::infinity();
	vmin[2] = std::numeric_limits< float >::infinity();

	vmax[0] = -std::numeric_limits< float >::infinity();
	vmax[1] = -std::numeric_limits< float >::infinity();
	vmax[2] = -std::numeric_limits< float >::infinity();

	typedef uint32_t BigIndex;
	typedef uint16_t CompactIndex;

	index_type = GL_UNSIGNED_INT; // BigIndex GL mapping
	const GLenum compact_index_type = GL_UNSIGNED_SHORT; // CompactIndex GL mapping

	while (true)
	{
		unsigned nv = 0;

		if (1 != fscanf(file(), "%u", &nv) || 0 == nv)
			break;

		scoped_ptr< void, generic_free > finish_vb(vb_total);
		scoped_ptr< void, generic_free > finish_ib(ib_total);

		if (uint64_t(nv) + nv_total > uint64_t(1) + BigIndex(-1))
		{
			stream::cerr << __FUNCTION__ << " encountered too many vertices in '" << filename << "'\n";
			return false;
		}

		finish_vb.reset();
		const size_t sizeof_vb = sizeof(float) * NUM_FLOATS_T * (nv + nv_total);
		scoped_ptr< void, generic_free > vb(realloc(vb_total, sizeof_vb));

		if (0 == vb())
			return false;

		for (unsigned i = 0; i < nv; ++i)
		{
			float (&vi)[NUM_FLOATS_T] =
				reinterpret_cast< float (*)[NUM_FLOATS_T] >(vb())[i + nv_total];

			if (NUM_FLOATS_T != fscanf_generic(file(), vi))
				return false;

			vmin[0] = min(vi[0], vmin[0]);
			vmin[1] = min(vi[1], vmin[1]);
			vmin[2] = min(vi[2], vmin[2]);

			vmax[0] = max(vi[0], vmax[0]);
			vmax[1] = max(vi[1], vmax[1]);
			vmax[2] = max(vi[2], vmax[2]);
		}

		unsigned nf = 0;

		if (1 != fscanf(file(), "%u", &nf) || 0 == nf)
			return false;

		finish_ib.reset();
		const size_t sizeof_ib = sizeof(BigIndex) * NUM_INDICES_T * (nf + nf_total);
		scoped_ptr< void, generic_free > ib(realloc(ib_total, sizeof_ib));

		if (0 == ib())
			return false;

		for (unsigned i = 0; i < nf; ++i)
		{
			BigIndex (&fi)[NUM_INDICES_T] =
				reinterpret_cast< BigIndex (*)[NUM_INDICES_T] >(ib())[i + nf_total];

			if (NUM_INDICES_T != fscanf_generic(file(), fi))
				return false;

			for (unsigned i = 0; i < NUM_INDICES_T; ++i)
				fi[i] += nv_total - MESH_INDEX_START;
		}

		vb_total = vb();
		vb.reset();

		ib_total = ib();
		ib.reset();

		nv_total += nv;
		nf_total += nf;
	}

	if (0 == nv_total || 0 == nf_total)
		return false;

	stream::cout << "number of vertices: " << nv_total <<
		"\nnumber of indices: " << nf_total * NUM_INDICES_T << '\n';

	size_t sizeof_index = sizeof(BigIndex);

	// compact index integral type if possible
	if (sizeof_index > sizeof(CompactIndex) &&
		uint64_t(1) + CompactIndex(-1) >= nv_total)
	{
		sizeof_index = sizeof(CompactIndex);

		const size_t sizeof_ib = sizeof_index * NUM_INDICES_T * nf_total;
		CompactIndex (* const ib)[NUM_INDICES_T] =
			reinterpret_cast< CompactIndex (*)[NUM_INDICES_T] >(malloc(sizeof_ib));

		if (0 == ib)
			return false;

		for (unsigned i = 0; i < nf_total; ++i)
		{
			BigIndex (&fi)[NUM_INDICES_T] =
				reinterpret_cast< BigIndex (*)[NUM_INDICES_T] >(ib_total)[i];

			for (unsigned j = 0; j < NUM_INDICES_T; ++j)
				ib[i][j] = CompactIndex(fi[j]);
		}

		free(ib_total);
		ib_total = ib;

		index_type = compact_index_type;
	}

	const size_t sizeof_vb = sizeof(float) * NUM_FLOATS_T * nv_total;
	const size_t sizeof_ib = sizeof_index * NUM_INDICES_T * nf_total;

	glBindBuffer(GL_ARRAY_BUFFER, vbo_arr);
	glBufferData(GL_ARRAY_BUFFER, sizeof_vb, vb_total, GL_STATIC_DRAW);

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, vbo_idx);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof_ib, ib_total, GL_STATIC_DRAW);

	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

	free(vb_total);
	free(ib_total);

	num_faces = nf_total;

	return true;
}

bool
fill_indexed_trilist_from_file_P(
	const char* const filename,
	const GLuint vbo_arr,
	const GLuint vbo_idx,
	unsigned& num_faces,
	GLenum& index_type,
	float (&vmin)[3],
	float (&vmax)[3])
{
	return fill_indexed_facelist_from_file< 3, 3 >(
		filename,
		vbo_arr,
		vbo_idx,
		num_faces,
		index_type,
		vmin,
		vmax);
}

bool
fill_indexed_trilist_from_file_PN(
	const char* const filename,
	const GLuint vbo_arr,
	const GLuint vbo_idx,
	unsigned& num_faces,
	GLenum& index_type,
	float (&vmin)[3],
	float (&vmax)[3])
{
	return fill_indexed_facelist_from_file< 6, 3 >(
		filename,
		vbo_arr,
		vbo_idx,
		num_faces,
		index_type,
		vmin,
		vmax);
}

bool
fill_indexed_trilist_from_file_PN2(
	const char* const filename,
	const GLuint vbo_arr,
	const GLuint vbo_idx,
	unsigned& num_faces,
	GLenum& index_type,
	float (&vmin)[3],
	float (&vmax)[3])
{
	return fill_indexed_facelist_from_file< 8, 3 >(
		filename,
		vbo_arr,
		vbo_idx,
		num_faces,
		index_type,
		vmin,
		vmax);
}

bool
fill_indexed_trilist_from_file_ABE(
	const char* const filename,
	const GLuint vbo_arr,
	const GLuint vbo_idx,
	const uintptr_t (&semantics_offset)[4],
	unsigned& num_faces,
	GLenum& index_type,
	float (&bmin)[3],
	float (&bmax)[3])
{
	assert(filename);

	scoped_ptr< FILE, scoped_functor > file(fopen(filename, "rb"));

	if (0 == file())
	{
		stream::cerr << "error: failure at fopen '" << filename << "'\n";
		return false;
	}

	uint32_t magic;
	uint32_t version;

	if (1 != fread(&magic, sizeof(magic), 1, file()) ||
		1 != fread(&version, sizeof(version), 1, file()))
	{
		stream::cerr << "error: failure at fread '" << filename << "'\n";
		return false;
	}

	stream::cout << "mesh magic, version: 0x" << stream::hex << stream::setw(8) << stream::setfill('0') <<
		magic << stream::dec << ", " << version << '\n';

	static const uint32_t sMagic = 0x6873656d;
	static const uint32_t sVersion = 100;

	if (magic != sMagic || version != sVersion)
	{
		stream::cerr << "error: mesh of unknown magic/version\n";
		return false;
	}

	uint8_t softwareSkinning;
	uint16_t primitiveType;

	if (1 != fread(&softwareSkinning, sizeof(softwareSkinning), 1, file()) ||
		1 != fread(&primitiveType, sizeof(primitiveType), 1, file()))
	{
		stream::cerr << "error: failure at fread '" << filename << "'\n";
		return false;
	}

	if (softwareSkinning)
	{
		stream::cerr << "error: mesh uses software skinning\n";
		return false;
	}

	enum PrimitiveType
	{
		PT_TRIANGLE_LIST	= 4,
	};

	enum AttribType
	{
		AT_FLOAT1			= 0,
		AT_FLOAT2			= 1,
		AT_FLOAT3			= 2,
		AT_FLOAT4			= 3,
		AT_UBYTE4			= 9
	};

	enum AttribSemantic
	{
		AS_POSITION			= 1,
		AS_BLEND_WEIGHTS	= 2,
		AS_NORMAL			= 4,
		AS_COLOR			= 5,
		AS_TEXTURE_COORD	= 7,
		AS_BITANGENT		= 8,
		AS_TANGENT			= 9	
	};

	enum IndexType
	{
		INDEX_16BIT			= 0,
		INDEX_32BIT			= 1
	};

	const PrimitiveType type = (PrimitiveType) primitiveType;
	assert(type == PT_TRIANGLE_LIST); (void) type;

	uint16_t num_attr;

	if (1 != fread(&num_attr, sizeof(num_attr), 1, file()))
	{
		stream::cerr << "error: failure at fread '" << filename << "'\n";
		return false;
	}

	stream::cout << "vertex attributes: " << num_attr << '\n';

	if (4 != num_attr)
	{
		stream::cerr << "error: mesh of unsupported number of attributes\n";
		return false;
	}

	uint16_t buffer_interest = uint16_t(-1);

	for (unsigned i = 0; i < num_attr; ++i)
	{ 
		uint16_t src_buffer;
		uint16_t type;
		uint16_t semantic;
		uint16_t offset;
		uint16_t index;

		if (1 != fread(&src_buffer, sizeof(src_buffer), 1, file()) ||
			1 != fread(&type, sizeof(type), 1, file()) ||
			1 != fread(&semantic, sizeof(semantic), 1, file()) ||
			1 != fread(&offset, sizeof(offset), 1, file()) ||
			1 != fread(&index, sizeof(index), 1, file()))
		{
			stream::cerr << "error: failure at fread '" << filename << "'\n";
			return false;
		}

		if (0 != index)
		{
			stream::cerr << "error: mesh has secondary attributes\n";
			return false;
		}

		switch (AttribSemantic(semantic)) {
		case AS_POSITION:
			if (AT_FLOAT3 != (AttribType) type || semantics_offset[0] != offset ||
				(uint16_t(-1) != buffer_interest && buffer_interest != src_buffer))
			{
				stream::cerr << "error: mesh has POSITION attribute of unsupported layout\n";
				return false;
			}
			buffer_interest = src_buffer;
			break;

		case AS_BLEND_WEIGHTS:
			if (AT_FLOAT4 != (AttribType) type || semantics_offset[1] != offset ||
				(uint16_t(-1) != buffer_interest && buffer_interest != src_buffer))
			{
				stream::cerr << "error: mesh has BLEND_WEIGHTS attribute of unsupported layout\n";
				return false;
			}
			buffer_interest = src_buffer;
			break;

		case AS_NORMAL:
			if (AT_FLOAT3 != (AttribType) type || semantics_offset[2] != offset ||
				(uint16_t(-1) != buffer_interest && buffer_interest != src_buffer))
			{
				stream::cerr << "error: mesh has NORMAL attribute of unsupported layout\n";
				return false;
			}
			buffer_interest = src_buffer;
			break;

		case AS_TEXTURE_COORD:
			if (AT_FLOAT2 != (AttribType) type || semantics_offset[3] != offset ||
				(uint16_t(-1) != buffer_interest && buffer_interest != src_buffer))
			{
				stream::cerr << "error: mesh has TEXTURE_COORD attribute of unsupported layout\n";
				return false;
			}
			buffer_interest = src_buffer;
			break;

		default:
			stream::cerr << "error: mesh has attribute of unsupported semantics; bailing out\n";
			return false;
		}
	}

	if (uint16_t(-1) == buffer_interest)
	{
		stream::cerr << "error: mesh has no usable attribute buffer; bailing out\n";
		return false;
	}

	uint32_t num_vertices;
	uint16_t num_buffers;

	if (1 != fread(&num_vertices, sizeof(num_vertices), 1, file()) ||
		1 != fread(&num_buffers, sizeof(num_buffers), 1, file()))
	{
		stream::cerr << "error: failure at fread '" << filename << "'\n";
		return false;
	}

	size_t sizeof_vb = 0;
	void* proto_vb = 0;

	for (unsigned i = 0; i < num_buffers; ++i)
	{ 
		uint16_t bind_index;
		uint16_t vertex_size;

		if (1 != fread(&bind_index, sizeof(bind_index), 1, file()) ||
			1 != fread(&vertex_size, sizeof(vertex_size), 1, file()))
		{
			stream::cerr << "error: failure at fread '" << filename << "'\n";
			return false;
		}

		sizeof_vb = size_t(vertex_size) * num_vertices;

		if (buffer_interest != i)
		{
			fseek(file(), sizeof_vb, SEEK_CUR);
			continue;
		}

		scoped_ptr< void, generic_free > buf(malloc(sizeof_vb));

		if (0 == buf() || 1 != fread(buf(), sizeof_vb, 1, file()))
		{
			stream::cerr << "error: failure reading attribute buffer from file '" << filename << "'\n";
			return false;
		}

		proto_vb = buf();
		buf.reset();
	}

	scoped_ptr< void, generic_free > vb(proto_vb);
	proto_vb = 0;

	uint16_t index_format;
	uint32_t num_indices;

	if (1 != fread(&index_format, sizeof(index_format), 1, file()) ||
		1 != fread(&num_indices, sizeof(num_indices), 1, file()))
	{
		stream::cerr << "error: failure at fread '" << filename << "'\n";
		return false;
	}

	if (0 == num_indices)
	{
		stream::cerr << "error: mesh not indexed\n";
		return false;
	}

	size_t sizeof_index = 0;

	switch ((IndexType) index_format)
	{
	case INDEX_16BIT:
		index_type = GL_UNSIGNED_SHORT;
		sizeof_index = sizeof(uint16_t);
		break;

	case INDEX_32BIT:
		index_type = GL_UNSIGNED_INT;
		sizeof_index = sizeof(uint32_t);
		break;

	default:
		stream::cerr << "error: unsupported index format\n";
		return false;
	}

	const size_t sizeof_ib = sizeof_index * num_indices;
	scoped_ptr< void, generic_free > ib(malloc(sizeof_ib));

	if (0 == ib() || 1 != fread(ib(), sizeof_ib, 1, file()))
	{
		stream::cerr << "error: failure reading index buffer from file '" << filename << "'\n";
		return false;
	}

	if (1 != fread(&bmin, sizeof(bmin), 1, file()) ||
		1 != fread(&bmax, sizeof(bmax), 1, file()))
	{
		stream::cerr << "error: failure at fread '" << filename << "'\n";
		return false;
	}

	stream::cout << "bbox min: (" <<
		bmin[0] << ", " <<
		bmin[1] << ", " <<
		bmin[2] << ")\nbbox max: (" <<
		bmax[0] << ", " <<
		bmax[1] << ", " <<
		bmax[2] << ")\n";

	float center[3];
	float radius;

	if (1 != fread(&center, sizeof(center), 1, file()) ||
		1 != fread(&radius, sizeof(radius), 1, file()))
	{
		stream::cerr << "error: failure at fread '" << filename << "'\n";
		return false;
	}

	stream::cout << "number of vertices: " << num_vertices <<
		"\nnumber of indices: " << num_indices << '\n';

	glBindBuffer(GL_ARRAY_BUFFER, vbo_arr);
	glBufferData(GL_ARRAY_BUFFER, sizeof_vb, vb(), GL_STATIC_DRAW);

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, vbo_idx);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof_ib, ib(), GL_STATIC_DRAW);

	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

	num_faces = num_indices / 3;

	return true;
}

////////////////////////////////////////////////////////////////////////////////
// OgreMesh deserializer from MeshSerializer_v1.41

enum VertexSharing {
	VERTEX_SHARING_UNKNOWN,
	VERTEX_SHARING_FALSE,
	VERTEX_SHARING_TRUE
};

enum IndexBitness {
	INDEX_BITNESS_UNKNOWN,
	INDEX_BITNESS_16,
	INDEX_BITNESS_32
};

struct Submesh {
	enum { buffer_capacity = 15 };

	size_t vertexCount;
	size_t indexCount;
	void* vertices[buffer_capacity];
	void* indices;

	Submesh()
	: vertexCount(0)
	, indexCount(0)
	, indices(0)
	{
		for (size_t i = 0; i < buffer_capacity; ++i)
			vertices[i] = 0;
	}
};

struct VertexElement {
	enum Type {
		VET_FLOAT1 = 0,
		VET_FLOAT2 = 1,
		VET_FLOAT3 = 2,
		VET_FLOAT4 = 3,
		VET_COLOUR = 4,
		VET_SHORT1 = 5,
		VET_SHORT2 = 6,
		VET_SHORT3 = 7,
		VET_SHORT4 = 8,
		VET_UBYTE4 = 9,
		VET_COLOUR_ARGB = 10,
		VET_COLOUR_ABGR = 11,
		VET_DOUBLE1 = 12,
		VET_DOUBLE2 = 13,
		VET_DOUBLE3 = 14,
		VET_DOUBLE4 = 15,
		VET_USHORT1 = 16,
		VET_USHORT2 = 17,
		VET_USHORT3 = 18,
		VET_USHORT4 = 19,
		VET_INT1 = 20,
		VET_INT2 = 21,
		VET_INT3 = 22,
		VET_INT4 = 23,
		VET_UINT1 = 24,
		VET_UINT2 = 25,
		VET_UINT3 = 26,
		VET_UINT4 = 27
	};
	enum Semantic {
		VES_POSITION = 1,
		VES_BLEND_WEIGHTS = 2,
		VES_BLEND_INDICES = 3,
		VES_NORMAL = 4,
		VES_DIFFUSE = 5,
		VES_SPECULAR = 6,
		VES_TEXTURE_COORDINATES = 7,
		VES_BITANGENT = 8,
		VES_TANGENT = 9
	};

	static const size_t size_from_type(const Type ty) {
		switch (ty) {
		case VET_FLOAT1:
			return sizeof(float);
		case VET_FLOAT2:
			return sizeof(float[2]);
		case VET_FLOAT3:
			return sizeof(float[3]);
		case VET_FLOAT4:
			return sizeof(float[4]);
		case VET_COLOUR:
			return sizeof(uint32_t);
		case VET_SHORT1:
			return sizeof(int16_t);
		case VET_SHORT2:
			return sizeof(int16_t[2]);
		case VET_SHORT3:
			return sizeof(int16_t[3]);
		case VET_SHORT4:
			return sizeof(int16_t[4]);
		case VET_UBYTE4:
			return sizeof(uint8_t[4]);
		case VET_COLOUR_ARGB:
			return sizeof(uint32_t);
		case VET_COLOUR_ABGR:
			return sizeof(uint32_t);
		case VET_DOUBLE1:
			return sizeof(double);
		case VET_DOUBLE2:
			return sizeof(double[2]);
		case VET_DOUBLE3:
			return sizeof(double[3]);
		case VET_DOUBLE4:
			return sizeof(double[4]);
		case VET_USHORT1:
			return sizeof(uint16_t);
		case VET_USHORT2:
			return sizeof(uint16_t[2]);
		case VET_USHORT3:
			return sizeof(uint16_t[3]);
		case VET_USHORT4:
			return sizeof(uint16_t[4]);
		case VET_INT1:
			return sizeof(int32_t);
		case VET_INT2:
			return sizeof(int32_t[2]);
		case VET_INT3:
			return sizeof(int32_t[3]);
		case VET_INT4:
			return sizeof(int32_t[4]);
		case VET_UINT1:
			return sizeof(uint32_t);
		case VET_UINT2:
			return sizeof(uint32_t[2]);
		case VET_UINT3:
			return sizeof(uint32_t[3]);
		case VET_UINT4:
			return sizeof(uint32_t[4]);
		}

		assert(0);
		return 0;
	}

	static const char* string_from_type(const Type ty) {
		switch (ty) {
		case VET_FLOAT1:
			return "VET_FLOAT1";
		case VET_FLOAT2:
			return "VET_FLOAT2";
		case VET_FLOAT3:
			return "VET_FLOAT3";
		case VET_FLOAT4:
			return "VET_FLOAT4";
		case VET_COLOUR:
			return "VET_COLOUR";
		case VET_SHORT1:
			return "VET_SHORT1";
		case VET_SHORT2:
			return "VET_SHORT2";
		case VET_SHORT3:
			return "VET_SHORT3";
		case VET_SHORT4:
			return "VET_SHORT4";
		case VET_UBYTE4:
			return "VET_UBYTE4";
		case VET_COLOUR_ARGB:
			return "VET_COLOUR_ARGB";
		case VET_COLOUR_ABGR:
			return "VET_COLOUR_ABGR";
		case VET_DOUBLE1:
			return "VET_DOUBLE1";
		case VET_DOUBLE2:
			return "VET_DOUBLE2";
		case VET_DOUBLE3:
			return "VET_DOUBLE3";
		case VET_DOUBLE4:
			return "VET_DOUBLE4";
		case VET_USHORT1:
			return "VET_USHORT1";
		case VET_USHORT2:
			return "VET_USHORT2";
		case VET_USHORT3:
			return "VET_USHORT3";
		case VET_USHORT4:
			return "VET_USHORT4";
		case VET_INT1:
			return "VET_INT1";
		case VET_INT2:
			return "VET_INT2";
		case VET_INT3:
			return "VET_INT3";
		case VET_INT4:
			return "VET_INT4";
		case VET_UINT1:
			return "VET_UINT1";
		case VET_UINT2:
			return "VET_UINT2";
		case VET_UINT3:
			return "VET_UINT3";
		case VET_UINT4:
			return "VET_UINT4";
		}

		assert(0);
		return "unknown-type";
	}
	static const char* string_from_semantic(const Semantic se) {
		switch (se) {
		case VES_POSITION:
			return "VES_POSITION";
		case VES_BLEND_WEIGHTS:
			return "VES_BLEND_WEIGHTS";
		case VES_BLEND_INDICES:
			return "VES_BLEND_INDICES";
		case VES_NORMAL:
			return "VES_NORMAL";
		case VES_DIFFUSE:
			return "VES_DIFFUSE";
		case VES_SPECULAR:
			return "VES_SPECULAR";
		case VES_TEXTURE_COORDINATES:
			return "VES_TEXTURE_COORDINATES";
		case VES_BITANGENT:
			return "VES_BITANGENT";
		case VES_TANGENT:
			return "VES_TANGENT";
		}

		assert(0);
		return "unknown-semantic";
	}

	uint16_t source;
	uint16_t type;
	uint16_t semantic;
	uint16_t offset;
	uint16_t index;

	VertexElement()
	: source(-1)
	{}

	bool empty() const {
		return uint16_t(-1) == source;
	}

	bool operator ==(const VertexElement& other) const {
		return
			source   == other.source &&
			type     == other.type &&
			semantic == other.semantic &&
			offset   == other.offset &&
			index    == other.index;
	}

	bool operator !=(const VertexElement& other) const {
		return !((*this) == other);
	}

	void print(FILE* out) const {
		fprintf(out,
			"\tsource: %hu\n"
			"\ttype: %s\n"
			"\tsemantic: %s\n"
			"\toffset: %hu\n"
			"\tindex: %hu\n",
			source,
			string_from_type(Type(type)),
			string_from_semantic(Semantic(semantic)),
			offset,
			index);
	}
};

struct VertexDecl {
	enum { capacity = 16 };

	size_t count;
	VertexElement elem[capacity];

	VertexDecl()
	: count(0)
	{}
};

static bool skipChunkOgre(
	FILE* file,
	uint32_t& chunk_size) // remaining size of parent chunk
{
	uint16_t skip_chunk_id;

	if (1 != fread(&skip_chunk_id, sizeof(skip_chunk_id), 1, file)) {
		fprintf(stderr, "%s encountered non-skeleton link\n", __FUNCTION__);
		return false;
	}

	uint32_t skip_chunk_size;
	if (1 != fread(&skip_chunk_size, sizeof(skip_chunk_size), 1, file)) {
		fprintf(stderr, "%s could not read chunk size\n", __FUNCTION__);
		return false;
	}

	if (skip_chunk_size > chunk_size) {
		fprintf(stderr, "%s chunk size mismatch\n", __FUNCTION__);
		return false;
	}

	chunk_size -= skip_chunk_size;

	return 0 == fseek(file, skip_chunk_size - (sizeof(skip_chunk_id) + sizeof(skip_chunk_size)), SEEK_CUR);
}

static bool loadVertexElemOgre(
	FILE* file,
	uint32_t& chunk_size, // remaining size of the chunk
	VertexElement& elem)
{
	uint16_t element_chunk_id;

	if (1 != fread(&element_chunk_id, sizeof(element_chunk_id), 1, file) || 0x5110 != element_chunk_id) {
		fprintf(stderr, "%s encountered non-element\n", __FUNCTION__);
		return false;
	}

	uint32_t element_chunk_size;
	if (1 != fread(&element_chunk_size, sizeof(element_chunk_size), 1, file)) {
		fprintf(stderr, "%s could not read chunk size\n", __FUNCTION__);
		return false;
	}

	if (element_chunk_size > chunk_size) {
		fprintf(stderr, "%s chunk size mismatch\n", __FUNCTION__);
		return false;
	}

	chunk_size -= element_chunk_size;

	VertexElement incoming;
	if (1 != fread(&incoming, sizeof(incoming), 1, file)) {
		fprintf(stderr, "%s could not read chunk content\n", __FUNCTION__);
		return false;
	}

	const uint32_t chunk_dec = sizeof(element_chunk_id) + sizeof(element_chunk_size) + sizeof(incoming);

	if (chunk_dec != element_chunk_size) {
		fprintf(stderr, "%s chunk size mismatch\n", __FUNCTION__);
		return false;
	}

	if (!elem.empty()) {
		if (incoming != elem) {
			fprintf(stderr, "%s encountered unprecedented vertex element\n", __FUNCTION__);
			return false;
		}
	}
	else {
		elem = incoming;
		incoming.print(stderr);
	}

	return true;
}

static bool loadVertexDeclOgre(
	FILE* file,
	uint32_t& chunk_size, // remaining size of the chunk
	VertexDecl& vertDecl)
{
	uint16_t declaration_chunk_id;

	if (1 != fread(&declaration_chunk_id, sizeof(declaration_chunk_id), 1, file) || 0x5100 != declaration_chunk_id) {
		fprintf(stderr, "%s encountered non-declaration\n", __FUNCTION__);
		return false;
	}

	uint32_t declaration_chunk_size;
	if (1 != fread(&declaration_chunk_size, sizeof(declaration_chunk_size), 1, file)) {
		fprintf(stderr, "%s could not read chunk size\n", __FUNCTION__);
		return false;
	}

	if (declaration_chunk_size > chunk_size) {
		fprintf(stderr, "%s chunk size mismatch\n", __FUNCTION__);
		return false;
	}

	chunk_size -= declaration_chunk_size;

	const uint32_t chunk_dec = sizeof(declaration_chunk_id) + sizeof(declaration_chunk_size);

	if (chunk_dec > declaration_chunk_size) {
		fprintf(stderr, "%s chunk size mismatch\n", __FUNCTION__);
		return false;
	}

	declaration_chunk_size -= chunk_dec;
	size_t declCount = 0;

	while (declaration_chunk_size) {
		uint16_t id;
		if (1 != fread(&id, sizeof(id), 1, file)) {
			fprintf(stderr, "%s could not read chunk id\n", __FUNCTION__);
			return false;
		}
		switch (id) {
		case 0x5110: // GEOMETRY_VERTEX_ELEMENT
			if (vertDecl.capacity == declCount) {
				fprintf(stderr, "%s encountered too many vertex elements\n", __FUNCTION__);
				return false;
			}
			if (!loadVertexElemOgre(file, declaration_chunk_size, vertDecl.elem[declCount++]))
				return false;
			break;
		default:
			fprintf(stderr, "%s encountered unhandled chunk id 0x%04hx; skipping\n", __FUNCTION__, id);
			if (!skipChunkOgre(file, declaration_chunk_size))
				return false;
			break;
		}
	}

	if (0 != vertDecl.count) {
		if (declCount != vertDecl.count) {
			fprintf(stderr, "%s encountered unprecedented vertex element count\n", __FUNCTION__);
			return false;
		}
	}
	else
		vertDecl.count = declCount;

	return true;
}

static bool loadVertexBufferOgre(
	FILE* file,
	uint32_t& chunk_size, // remaining size of the chunk
	Submesh& submesh)
{
	uint16_t buffer_chunk_id;

	if (1 != fread(&buffer_chunk_id, sizeof(buffer_chunk_id), 1, file) || 0x5200 != buffer_chunk_id) {
		fprintf(stderr, "%s encountered non-buffer\n", __FUNCTION__);
		return false;
	}

	uint32_t buffer_chunk_size;
	if (1 != fread(&buffer_chunk_size, sizeof(buffer_chunk_size), 1, file)) {
		fprintf(stderr, "%s could not read chunk size\n", __FUNCTION__);
		return false;
	}

	if (buffer_chunk_size > chunk_size) {
		fprintf(stderr, "%s chunk size mismatch\n", __FUNCTION__);
		return false;
	}

	chunk_size -= buffer_chunk_size;

	uint16_t bindIndex;
	if (1 != fread(&bindIndex, sizeof(bindIndex), 1, file)) {
		fprintf(stderr, "%s could not read bind index\n", __FUNCTION__);
		return false;
	}

	if (bindIndex > Submesh::buffer_capacity) {
		fprintf(stderr, "%s encountered out-of-bounds bind index\n", __FUNCTION__);
		return false;
	}

	if (0 != submesh.vertices[bindIndex]) {
		fprintf(stderr, "%s encountered pre-occupied bind index\n", __FUNCTION__);
		return false;
	}

	uint16_t vertexSize;
	if (1 != fread(&vertexSize, sizeof(vertexSize), 1, file)) {
		fprintf(stderr, "%s could not read vertex size\n", __FUNCTION__);
		return false;
	}

	const uint32_t chunk_dec = sizeof(buffer_chunk_id) + sizeof(buffer_chunk_size) + sizeof(bindIndex) + sizeof(vertexSize);

	if (chunk_dec > buffer_chunk_size) {
		fprintf(stderr, "%s chunk size mismatch\n", __FUNCTION__);
		return false;
	}

	buffer_chunk_size -= chunk_dec;

	uint16_t buffer_data_chunk_id;

	if (1 != fread(&buffer_data_chunk_id, sizeof(buffer_data_chunk_id), 1, file) || 0x5210 != buffer_data_chunk_id) {
		fprintf(stderr, "%s encountered non-buffer-data\n", __FUNCTION__);
		return false;
	}

	uint32_t buffer_data_chunk_size;
	if (1 != fread(&buffer_data_chunk_size, sizeof(buffer_data_chunk_size), 1, file)) {
		fprintf(stderr, "%s could not read buffer-data chunk size\n", __FUNCTION__);
		return false;
	}

	if (buffer_data_chunk_size != buffer_chunk_size) {
		fprintf(stderr, "%s buffer-data chunk size mismatch\n", __FUNCTION__);
		return false;
	}

	const uint32_t buffer_data_chunk_dec = sizeof(buffer_data_chunk_id) + sizeof(buffer_data_chunk_size);

	if (buffer_data_chunk_dec > buffer_data_chunk_size) {
		fprintf(stderr, "%s buffer-data chunk size mismatch\n", __FUNCTION__);
		return false;
	}

	buffer_data_chunk_size -= buffer_data_chunk_dec;

	if (uint32_t(vertexSize) * submesh.vertexCount != buffer_data_chunk_size) {
		fprintf(stderr, "%s encountered vertex size vs buffer-data chunk size mismatch\n", __FUNCTION__);
		return false;
	}

	scoped_ptr< void, generic_free > vb(malloc(buffer_data_chunk_size));

	if (1 != fread(vb(), buffer_data_chunk_size, 1, file)) {
		fprintf(stderr, "%s could not buffer data\n", __FUNCTION__);
		return false;
	}

	submesh.vertices[bindIndex] = vb();
	vb.reset();

	return true;
}

static bool loadGeometryOgre(
	FILE* file,
	uint32_t& chunk_size, // remaining size of the chunk
	Submesh& submesh,
	VertexDecl& vertDecl)
{
	uint16_t geometry_chunk_id;

	if (1 != fread(&geometry_chunk_id, sizeof(geometry_chunk_id), 1, file) || 0x5000 != geometry_chunk_id) {
		fprintf(stderr, "%s encountered non-geometry\n", __FUNCTION__);
		return false;
	}

	uint32_t geometry_chunk_size;
	if (1 != fread(&geometry_chunk_size, sizeof(geometry_chunk_size), 1, file)) {
		fprintf(stderr, "%s could not read chunk size\n", __FUNCTION__);
		return false;
	}

	if (geometry_chunk_size > chunk_size) {
		fprintf(stderr, "%s chunk size mismatch\n", __FUNCTION__);
		return false;
	}

	chunk_size -= geometry_chunk_size;

	uint32_t vertexCount;
	if (1 != fread(&vertexCount, sizeof(vertexCount), 1, file)) {
		fprintf(stderr, "%s could not read vertex count\n", __FUNCTION__);
		return false;
	}

	fprintf(stderr, "\tvertex count: %u\n", vertexCount);
	submesh.vertexCount = vertexCount;

	const uint32_t chunk_dec = sizeof(geometry_chunk_id) + sizeof(geometry_chunk_size) + sizeof(vertexCount);

	if (chunk_dec > geometry_chunk_size) {
		fprintf(stderr, "%s chunk size mismatch\n", __FUNCTION__);
		return false;
	}

	geometry_chunk_size -= chunk_dec;

	while (geometry_chunk_size) {
		uint16_t id;
		if (1 != fread(&id, sizeof(id), 1, file)) {
			fprintf(stderr, "%s could not read chunk id\n", __FUNCTION__);
			return false;
		}
		switch (id) {
		case 0x5100: // GEOMETRY_VERTEX_DECLARATION
			if (!loadVertexDeclOgre(file, geometry_chunk_size, vertDecl))
				return false;
			break;
		case 0x5200: // GEOMETRY_VERTEX_BUFFER
		default:
			if (!loadVertexBufferOgre(file, geometry_chunk_size, submesh))
				return false;
			break;
		}
	}

	return true;
}

static bool loadSubmeshOgre(
	FILE* file,
	uint32_t& chunk_size, // remaining size of the chunk
	VertexSharing& vertSharing,
	IndexBitness& indexBitness,
	Submesh& submesh,
	VertexDecl& vertDecl)
{
	uint16_t submesh_chunk_id;

	if (1 != fread(&submesh_chunk_id, sizeof(submesh_chunk_id), 1, file) || 0x4000 != submesh_chunk_id) {
		fprintf(stderr, "%s encountered non-submesh\n", __FUNCTION__);
		return false;
	}

	uint32_t submesh_chunk_size;
	if (1 != fread(&submesh_chunk_size, sizeof(submesh_chunk_size), 1, file)) {
		fprintf(stderr, "%s could not read chunk size\n", __FUNCTION__);
		return false;
	}

	if (submesh_chunk_size > chunk_size) {
		fprintf(stderr, "%s chunk size mismatch\n", __FUNCTION__);
		return false;
	}

	chunk_size -= submesh_chunk_size;

	size_t name_len = 0;
	char name[1024];
	while (name_len < sizeof(name) / sizeof(name[0])) {
		if (1 != fread(name + name_len, sizeof(name[0]), 1, file)) {
			fprintf(stderr, "%s could not read name\n", __FUNCTION__);
			return false;
		}
		if (name[name_len] == 0xa) {
			name[name_len] = '\0';
			break;
		}
		++name_len;
	}

	if (name_len == sizeof(name) / sizeof(name[0])) {
		fprintf(stderr, "%s encountered too long name\n", __FUNCTION__);
		return false;
	}

	uint8_t sharedVert;
	if (1 != fread(&sharedVert, sizeof(sharedVert), 1, file)) {
		fprintf(stderr, "%s could not read shared-vertices flag\n", __FUNCTION__);
		return false;
	}

	const VertexSharing sharing = sharedVert ? VERTEX_SHARING_TRUE : VERTEX_SHARING_FALSE;

	switch (vertSharing) {
	case VERTEX_SHARING_UNKNOWN:
		vertSharing = sharing;
		break;
	case VERTEX_SHARING_FALSE:
	case VERTEX_SHARING_TRUE:
		if (vertSharing != sharing) {
			fprintf(stderr, "%s encountered unprecedented vertex sharing\n", __FUNCTION__);
			return false;
		}
		break;
	}

	uint32_t indexCount;
	if (1 != fread(&indexCount, sizeof(indexCount), 1, file)) {
		fprintf(stderr, "%s could not read index count\n", __FUNCTION__);
		return false;
	}

	uint8_t index32bit;
	if (1 != fread(&index32bit, sizeof(index32bit), 1, file)) {
		fprintf(stderr, "%s could not read index bitness flag\n", __FUNCTION__);
		return false;
	}

	fprintf(stderr, "%s\n\tshared vert: %hhu\n\tindex count: %u\n\tindex 32-bit: %hhu\n",
		name, sharedVert, indexCount, index32bit);

	const IndexBitness bitness = index32bit ? INDEX_BITNESS_32 : INDEX_BITNESS_16;

	switch (indexBitness) {
	case INDEX_BITNESS_UNKNOWN:
		indexBitness = bitness;
		break;
	case INDEX_BITNESS_16:
	case INDEX_BITNESS_32:
		if (indexBitness != bitness) {
			fprintf(stderr, "%s encountered unprecedented index bitness\n", __FUNCTION__);
			return false;
		}
		break;
	}

	const size_t sizeof_index = indexBitness == INDEX_BITNESS_32 ? sizeof(uint32_t) : sizeof(uint16_t);
	const size_t sizeof_ib = sizeof_index * indexCount;
	scoped_ptr< void, generic_free > ib(malloc(sizeof_ib));

	if (1 != fread(ib(), sizeof_ib, 1, file)) {
		fprintf(stderr, "%s could not read index array\n", __FUNCTION__);
		return false;
	}

	const uint32_t chunk_dec = sizeof(submesh_chunk_id) + sizeof(submesh_chunk_size) + name_len + 1 + sizeof(sharedVert) +
		sizeof(indexCount) + sizeof(index32bit) + sizeof_ib;

	if (chunk_dec > submesh_chunk_size) {
		fprintf(stderr, "%s chunk size mismatch\n", __FUNCTION__);
		return false;
	}

	submesh_chunk_size -= chunk_dec;

	while (submesh_chunk_size) {
		uint16_t id;
		if (1 != fread(&id, sizeof(id), 1, file)) {
			fprintf(stderr, "%s could not read chunk id\n", __FUNCTION__);
			return false;
		}
		switch (id) {
		case 0x4010: // SUBMESH_OPERATION
			if (!skipChunkOgre(file, submesh_chunk_size))
				return false;
			break;
		case 0x4100: // SUBMESH_BONE_ASSIGNMENT
			if (!skipChunkOgre(file, submesh_chunk_size))
				return false;
			break;
		case 0x5000: // GEOMETRY
			if (!loadGeometryOgre(file, submesh_chunk_size, submesh, vertDecl))
				return false;
			break;
		default:
			fprintf(stderr, "%s encountered unhandled chunk id 0x%04hx; skipping\n", __FUNCTION__, id);
			if (!skipChunkOgre(file, submesh_chunk_size))
				return false;
			break;
		}
	}

	submesh.indexCount = indexCount;
	submesh.indices = ib();
	ib.reset();

	return true;
}

static bool loadMeshBoundsOgre(
	FILE* file,
	uint32_t& chunk_size, // remaining size of parent chunk
	float (&bmin)[3],
	float (&bmax)[3])
{
	uint16_t bounds_chunk_id;

	if (1 != fread(&bounds_chunk_id, sizeof(bounds_chunk_id), 1, file) || 0x9000 != bounds_chunk_id) {
		fprintf(stderr, "%s encountered non-bounds\n", __FUNCTION__);
		return false;
	}

	uint32_t bounds_chunk_size;
	if (1 != fread(&bounds_chunk_size, sizeof(bounds_chunk_size), 1, file)) {
		fprintf(stderr, "%s could not read chunk size\n", __FUNCTION__);
		return false;
	}

	if (bounds_chunk_size > chunk_size) {
		fprintf(stderr, "%s chunk size mismatch\n", __FUNCTION__);
		return false;
	}

	chunk_size -= bounds_chunk_size;

	if (1 != fread(bmin, sizeof(bmin), 1, file)) {
		fprintf(stderr, "%s could not read min bounds\n", __FUNCTION__);
		return false;
	}

	if (1 != fread(bmax, sizeof(bmax), 1, file)) {
		fprintf(stderr, "%s could not read max bounds\n", __FUNCTION__);
		return false;
	}

	float radius; // discarded
	if (1 != fread(&radius, sizeof(radius), 1, file)) {
		fprintf(stderr, "%s could not read radius\n", __FUNCTION__);
		return false;
	}

	return bounds_chunk_size == sizeof(bounds_chunk_id) + sizeof(bounds_chunk_size) + sizeof(bmin) + sizeof(bmax) + sizeof(radius);
}

static inline bool isfinite(const float c) {
	const uint32_t exp_mask = 0x7f800000;
	const uint32_t ic = reinterpret_cast< const uint32_t& >(c);
	return exp_mask != (ic & exp_mask);
}

bool
fill_indexed_trilist_from_file_Ogre(
	const char* const filename,
	const GLuint vbo_arr,
	const GLuint vbo_idx,
	const uintptr_t (&semantics_offset)[4],
	unsigned& num_faces,
	GLenum& index_type,
	float (&bmin)[3],
	float (&bmax)[3])
{
	assert(filename);
	scoped_ptr< FILE, scoped_functor > file(fopen(filename, "rb"));

	if (0 == file()) {
		stream::cerr << "error: failure at fopen '" << filename << "'\n";
		return false;
	}

	uint16_t mesh_chunk_id;
	if (1 != fread(&mesh_chunk_id, sizeof(mesh_chunk_id), 1, file()) || 0x3000 != mesh_chunk_id) {
		fprintf(stderr, "%s encountered non-mesh\n", __FUNCTION__);
		return false;
	}

	uint32_t mesh_chunk_size;
	if (1 != fread(&mesh_chunk_size, sizeof(mesh_chunk_size), 1, file())) {
		fprintf(stderr, "%s could not read chunk size\n", __FUNCTION__);
		return false;
	}

	uint8_t skelAnimated = 0;
	if (1 != fread(&skelAnimated, sizeof(skelAnimated), 1, file())) {
		fprintf(stderr, "%s could not read animated flag\n", __FUNCTION__);
		return false;
	}

	if (0 == skelAnimated) {
		fprintf(stderr, "%s encountered mesh sans skeletal animation\n", __FUNCTION__);
		return false;
	}

	mesh_chunk_size -= sizeof(mesh_chunk_id) + sizeof(mesh_chunk_size) + sizeof(skelAnimated);

	uint64_t vertexCount = 0;
	uint64_t indexCount = 0;
	VertexSharing vertSharing = VERTEX_SHARING_UNKNOWN;
	IndexBitness indexBitness = INDEX_BITNESS_UNKNOWN;
	const float inf = 1.f / 0.f;
	bmin[0] = bmin[1] = bmin[2] = inf;
	bmax[0] = bmax[1] = bmax[2] = -inf;
	std::vector< Submesh > submesh;
	VertexDecl vertDecl;

	while (mesh_chunk_size) {
		uint16_t id;
		if (1 != fread(&id, sizeof(id), 1, file())) {
			fprintf(stderr, "%s could not read chunk id\n", __FUNCTION__);
			return false;
		}

		Submesh smesh;

		switch (id) {
		case 0x4000: // SUBMESH
			if (!loadSubmeshOgre(file(), mesh_chunk_size, vertSharing, indexBitness, smesh, vertDecl))
				return false;
			vertexCount += smesh.vertexCount;
			indexCount += smesh.indexCount;
			submesh.push_back(smesh);
			break;

		case 0x9000: // MESH_BOUNDS
			if (!loadMeshBoundsOgre(file(), mesh_chunk_size, bmin, bmax))
				return false;
			break;

		default:
			fprintf(stderr, "%s encountered unhandled chunk id 0x%04hx; skipping\n", __FUNCTION__, id);

			if (!skipChunkOgre(file(), mesh_chunk_size))
				return false;
			break;
		}
	}

	if (!isfinite(bmin[0]) || !isfinite(bmin[1]) || !isfinite(bmin[2]) ||
		!isfinite(bmax[0]) || !isfinite(bmax[1]) || !isfinite(bmax[2])) {

		return false;
	}

	size_t sizeof_index = 0;
	switch (indexBitness) {
	case INDEX_BITNESS_16:
		index_type = GL_UNSIGNED_SHORT;
		sizeof_index = sizeof(uint16_t);
		break;

	case INDEX_BITNESS_32:
		index_type = GL_UNSIGNED_INT;
		sizeof_index = sizeof(uint32_t);
		break;
	}

	if (vertexCount > (uint64_t(1) << sizeof_index * 8)) {
		fprintf(stderr, "total vertex count of submeshes exceeds indexing capacity\n");
		return false;
	}

	// these are the semantics we care about currently and
	// this is the order we want them in in the output buffer
	enum {
		SEMANTIC_POS,
		SEMANTIC_BON,
		SEMANTIC_NRM,
		SEMANTIC_TXC,

		SEMANTIC_COUNT
	};

	uintptr_t src_semantics_size[SEMANTIC_COUNT] = { 0 };
	uintptr_t src_semantics_offset[SEMANTIC_COUNT];
	uint16_t src_semantics_buffer[SEMANTIC_COUNT];
	uint16_t src_buffer_stride[Submesh::buffer_capacity] = { 0 };
	size_t sizeof_vertex = sizeof(float[4]); // TODO compensate for missing bone attrib

	for (size_t i = 0; i < vertDecl.count; ++i) {
		assert(sizeof(src_buffer_stride) / sizeof(src_buffer_stride[0]) > vertDecl.elem[i].source);
		const size_t sizeof_type = VertexElement::size_from_type(VertexElement::Type(vertDecl.elem[i].type));
		src_buffer_stride[vertDecl.elem[i].source] += sizeof_type;

		switch (vertDecl.elem[i].semantic) {
		case VertexElement::VES_POSITION:
			src_semantics_size  [SEMANTIC_POS] = sizeof_type;
			src_semantics_offset[SEMANTIC_POS] = vertDecl.elem[i].offset;
			src_semantics_buffer[SEMANTIC_POS] = vertDecl.elem[i].source;
			sizeof_vertex += sizeof_type;
			break;
		case VertexElement::VES_BLEND_WEIGHTS:
		case VertexElement::VES_BLEND_INDICES:
			break;
		case VertexElement::VES_NORMAL:
			src_semantics_size  [SEMANTIC_NRM] = sizeof_type;
			src_semantics_offset[SEMANTIC_NRM] = vertDecl.elem[i].offset;
			src_semantics_buffer[SEMANTIC_NRM] = vertDecl.elem[i].source;
			sizeof_vertex += sizeof_type;
			break;
		case VertexElement::VES_DIFFUSE:
		case VertexElement::VES_SPECULAR:
			break;
		case VertexElement::VES_TEXTURE_COORDINATES:
			if (0 != vertDecl.elem[i].index) break;
			src_semantics_size  [SEMANTIC_TXC] = sizeof_type;
			src_semantics_offset[SEMANTIC_TXC] = vertDecl.elem[i].offset;
			src_semantics_buffer[SEMANTIC_TXC] = vertDecl.elem[i].source;
			sizeof_vertex += sizeof_type;
			break;
		case VertexElement::VES_BITANGENT:
		case VertexElement::VES_TANGENT:
			break;
		default:
			fprintf(stderr, "unexpected vertex attrib semantic '%s'; bailing out\n",
				VertexElement::string_from_semantic(VertexElement::Semantic(vertDecl.elem[i].semantic)));
			break;
		}
	}

	assert(0 != src_semantics_size[SEMANTIC_POS]);
	assert(0 == src_semantics_size[SEMANTIC_BON]); // TODO missing bone attrib
	assert(0 != src_semantics_size[SEMANTIC_NRM]);
	assert(0 != src_semantics_size[SEMANTIC_TXC]);

	const GLsizeiptr sizeArr = GLsizeiptr(vertexCount) * sizeof_vertex;
	const GLsizeiptr sizeIdx = GLsizeiptr(indexCount) * sizeof_index;

	fprintf(stdout, "vertex size: %u\n", uint32_t(sizeof_vertex));

	glBindBuffer(GL_ARRAY_BUFFER,         vbo_arr);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, vbo_idx);

	glBufferData(GL_ARRAY_BUFFER,         sizeArr, 0, GL_STATIC_DRAW);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeIdx, 0, GL_STATIC_DRAW);

	void* bitsArr = glMapBufferOES(GL_ARRAY_BUFFER,         GL_WRITE_ONLY_OES);
	void* bitsIdx = glMapBufferOES(GL_ELEMENT_ARRAY_BUFFER, GL_WRITE_ONLY_OES);
	size_t offsIdx = 0;

	// gather the output buffers from the source buffers
 	for (std::vector< Submesh >::iterator it = submesh.begin(); it != submesh.end(); ++it) {
		// fill in the vertices
		const size_t subVertCount = it->vertexCount;
		int8_t* typedBitsArr = reinterpret_cast< int8_t* >(bitsArr);

		for (size_t i = 0; i < subVertCount; ++i) {
			{
				const size_t source = src_semantics_buffer[SEMANTIC_POS];
				memcpy(typedBitsArr + semantics_offset[SEMANTIC_POS],
					reinterpret_cast< int8_t* >(it->vertices[source]) + i * src_buffer_stride[source] + src_semantics_offset[SEMANTIC_POS], src_semantics_size[SEMANTIC_POS]);
			}
			{
				const size_t source = src_semantics_buffer[SEMANTIC_NRM];
				memcpy(typedBitsArr + semantics_offset[SEMANTIC_NRM],
					reinterpret_cast< int8_t* >(it->vertices[source]) + i * src_buffer_stride[source] + src_semantics_offset[SEMANTIC_NRM], src_semantics_size[SEMANTIC_NRM]);
			}
			{
				const size_t source = src_semantics_buffer[SEMANTIC_TXC];
				memcpy(typedBitsArr + semantics_offset[SEMANTIC_TXC],
					reinterpret_cast< int8_t* >(it->vertices[source]) + i * src_buffer_stride[source] + src_semantics_offset[SEMANTIC_TXC], src_semantics_size[SEMANTIC_TXC]);
			}
			{
				// patch in a default bone attrib (root of weight 1.0)
				const float root[4] = { 1.f, 0.f, 0.f, 0.f };
				memcpy(typedBitsArr + semantics_offset[SEMANTIC_BON], root, sizeof(root));
			}
			typedBitsArr += sizeof_vertex;
		}

		bitsArr = typedBitsArr;

		for (size_t i = 0; i < Submesh::buffer_capacity; ++i)
			if (0 != it->vertices[i])
				free(it->vertices[i]);

		// fill in the indices
		const size_t subIdxCount = it->indexCount;
		void* const indices = it->indices;

		if (INDEX_BITNESS_16 == indexBitness) {
			uint16_t* typedBitsIdx = reinterpret_cast< uint16_t* >(bitsIdx);

			for (size_t i = 0; i < subIdxCount; ++i)
				*typedBitsIdx++ = uint16_t(offsIdx + reinterpret_cast< const uint16_t* >(indices)[i]);

			bitsIdx = typedBitsIdx;
		}
		else {
			uint32_t* typedBitsIdx = reinterpret_cast< uint32_t* >(bitsIdx);

			for (size_t i = 0; i < subIdxCount; ++i)
				*typedBitsIdx++ = uint32_t(offsIdx + reinterpret_cast< const uint32_t* >(indices)[i]);

			bitsIdx = typedBitsIdx;
		}
		offsIdx += subVertCount;
		free(it->indices);
	}

	glUnmapBufferOES(GL_ARRAY_BUFFER);
	glUnmapBufferOES(GL_ELEMENT_ARRAY_BUFFER);

	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

	num_faces = uint32_t(indexCount) / 3;

	fprintf(stdout, "number of vertices: %u\nnumber of indices: %u\n",
		uint32_t(vertexCount), uint32_t(indexCount));

	return true;
}

} // namespace util
