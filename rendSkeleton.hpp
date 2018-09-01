#ifndef	rend_skeleton_H__
#define	rend_skeleton_H__

#include "vectbase.hpp"
#include "vectsimd.hpp"

#if __MINGW32__
#include <malloc.h>
#endif
#include <stdint.h>
#include <stdlib.h>
#include <string>
#include <vector>

namespace rend {

struct BonePositionKey
{
	typedef simd::vect3 ValueType;

	float time;
	ValueType value;
};

struct BoneOrientationKey
{
	typedef simd::quat ValueType;

	float time;
	ValueType value;
};

struct BoneScaleKey
{
	typedef simd::vect3 ValueType;

	float time;
	ValueType value;
};

// provide proper alignment to simd members of bone keys when latter reside in STL containers
template< typename T >
struct allocator
{
	typedef T value_type;
	typedef T* pointer;
	typedef const T* const_pointer;
	typedef T& reference;
	typedef const T& const_reference;
	typedef std::size_t size_type;
	typedef std::ptrdiff_t difference_type;

	template< class U > struct rebind { typedef allocator<U> other; };

	pointer address(reference x) const {
		return &x;
	}

	const_pointer address(const_reference x) const {
		return &x;
	}

	pointer allocate(size_type n, std::allocator<void>::const_pointer hint = 0);

	void deallocate(T* p, size_type n) {
#if __MINGW32__
		__mingw_aligned_free(p);
#else
		free(p);
#endif
	}

	size_type max_size() const {
		return size_type(-1) / sizeof(value_type);
	}

	void construct(pointer p, const_reference val) {
		new (p) T(val);
	}

	void destroy(pointer p) {
		p->~T();
	}
};

template< typename T >
inline typename allocator< T >::pointer allocator< T >::allocate(
	typename allocator< T >::size_type n,
	std::allocator<void>::const_pointer) {

	const size_t size = n * sizeof(T);
	const size_t alignment = sizeof(typename T::ValueType);
	const base::compile_assert< 0 == (alignment & (alignment - 1)) > assert_alignment;
	void* ret = 0;
#if __MINGW32__
	ret = __mingw_aligned_malloc(size, alignment);
#else
	const int res = posix_memalign(&ret, alignment, size); (void) res;
#endif
	return reinterpret_cast< pointer >(ret);
}

// some c++11 compilers insist on the explicit definiton of == and != operators for the stateless allocators

inline bool operator==(
	const allocator< BonePositionKey >& lhs,
	const allocator< BonePositionKey >& rhs)
{
	return true;
}

inline bool operator!=(
	const allocator< BonePositionKey >& lhs,
	const allocator< BonePositionKey >& rhs)
{
	return false;
}

inline bool operator==(
	const allocator< BoneOrientationKey >& lhs,
	const allocator< BoneOrientationKey >& rhs)
{
	return true;
}

inline bool operator!=(
	const allocator< BoneOrientationKey >& lhs,
	const allocator< BoneOrientationKey >& rhs)
{
	return false;
}

inline bool operator==(
	const allocator< BoneScaleKey >& lhs,
	const allocator< BoneScaleKey >& rhs)
{
	return true;
}

inline bool operator!=(
	const allocator< BoneScaleKey >& lhs,
	const allocator< BoneScaleKey >& rhs)
{
	return false;
}

struct Bone
{
	simd::vect3     position;           // current position in parent space
	simd::quat      orientation;        // current orientation in parent space
	simd::vect3     scale;              // current scale in parent space
	uint8_t         parent_idx;         // 255 - no parent
	std::string     name;               // TODO: this has no relation to the render loop and belongs elsewhere

	simd::matx4     to_local;
	simd::matx4     to_model;
	bool            matx_valid;

	Bone()
	: position(0.f, 0.f, 0.f)
	, orientation(0.f, 0.f, 0.f, 1.f)
	, scale(1.f, 1.f, 1.f)
	, parent_idx(255)
	, matx_valid(false)
	{
		to_local.identity();
		to_model.identity();
	}
};


struct Track
{
	typedef std::vector< BonePositionKey, allocator< BonePositionKey > >       BonePositionKeys;
	typedef std::vector< BoneOrientationKey, allocator< BoneOrientationKey > > BoneOrientationKeys;
	typedef std::vector< BoneScaleKey, allocator< BoneScaleKey > >             BoneScaleKeys;

	uint8_t bone_idx;

	BonePositionKeys    position_key;
	BoneOrientationKeys orientation_key;
	BoneScaleKeys       scale_key;

	mutable unsigned position_last_key_idx;    // TODO: these do not belong here as we might want to source
	mutable unsigned orientation_last_key_idx; // more than one T from the same track; last keys' natural
	mutable unsigned scale_last_key_idx;       // occurrence would be one set per track per animated mesh
};

template < bool >
class alt_dense_matx4;

template <>
class alt_dense_matx4< false > : public base::matx< float, 4 >
{
public:
	alt_dense_matx4()
	{}

	alt_dense_matx4(
		const float c0, const float c1, const float c2, const float c3,
		const float c4, const float c5, const float c6, const float c7,
		const float c8, const float c9, const float ca, const float cb,
		const float cc, const float cd, const float ce, const float cf)
	{
		this->set(0, 0, c0);
		this->set(0, 1, c1);
		this->set(0, 2, c2);
		this->set(0, 3, c3);

		this->set(1, 0, c4);
		this->set(1, 1, c5);
		this->set(1, 2, c6);
		this->set(1, 3, c7);

		this->set(2, 0, c8);
		this->set(2, 1, c9);
		this->set(2, 2, ca);
		this->set(2, 3, cb);

		this->set(3, 0, cc);
		this->set(3, 1, cd);
		this->set(3, 2, ce);
		this->set(3, 3, cf);
	}

	typedef const float (& decast)[16];
	operator decast () const
	{
		return reinterpret_cast< const float (&)[16] >(*this);
	}
};

template <>
class alt_dense_matx4< true > : public simd::matx4
{
public:
	alt_dense_matx4()
	{}

	alt_dense_matx4(
		const float c0, const float c1, const float c2, const float c3,
		const float c4, const float c5, const float c6, const float c7,
		const float c8, const float c9, const float ca, const float cb,
		const float cc, const float cd, const float ce, const float cf)
	: simd::matx4(
		c0, c1, c2, c3,
		c4, c5, c6, c7,
		c8, c9, ca, cb,
		cc, cd, ce, cf)
	{}

	typedef const float (& decast)[16];
	operator decast () const
	{
		return reinterpret_cast< const float (&)[16] >(*this);
	}
};

typedef alt_dense_matx4< sizeof(simd::matx4) == sizeof(float[16]) > dense_matx4;


void
initBoneMatx(
	const unsigned bone_count,
	dense_matx4* bone_mat,
	Bone* bone,
	const unsigned bone_idx);


void
updateBoneMatx(
	const unsigned bone_count,
	dense_matx4* bone_mat,
	Bone* bone,
	const unsigned bone_idx);


void
updateRoot(
	Bone* root);


void
invalidateBoneMatx(
	const unsigned bone_count,
	Bone* bone,
	const unsigned bone_idx);


void
animateSkeleton(
	const unsigned bone_count,
	dense_matx4* bone_mat,
	Bone* bone,
	const std::vector< Track >& skeletal_animation,
	const float anim_time,
	Bone* root = 0);


void
resetSkeletonAnimProgress(
	const std::vector< Track >& skeletal_animation);


bool
loadSkeletonAnimationABE(
	const char* const filename,
	unsigned* count,
	dense_matx4* bone_mat,
	Bone* bone,
	std::vector< std::vector< Track > >& animations,
	std::vector< float >& durations);


bool
loadSkeletonAnimationOgre(
	const char* const filename,
	unsigned* count,
	dense_matx4* bone_mat,
	Bone* bone,
	std::vector< std::vector< Track > >& animations,
	std::vector< float >& durations);

} // namespace rend

#endif // rend_skeleton_H__
