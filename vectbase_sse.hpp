#ifndef vect_base_sse_H__
#define vect_base_sse_H__

#if __SSE2__ == 0
	#error SSE2 required
#endif

#if __SSE4_1__ == 1
	#include <smmintrin.h>
#else
	#include <emmintrin.h>
#endif

#include <cassert>
#include "vectbase.hpp"

namespace base {

inline static float get(
	const __m128 v,
	const size_t e)
{
	switch (e)
	{
	case 0:
		return _mm_cvtss_f32(v);
	case 1:
		return _mm_cvtss_f32(_mm_shuffle_ps(v, v, 0xe5));
	case 2:
		return _mm_cvtss_f32(_mm_shuffle_ps(v, v, 0xe6));
	case 3:
		return _mm_cvtss_f32(_mm_shuffle_ps(v, v, 0xe7));
	};

	assert(0);
	return 0;
}

inline static int32_t get(
	const __m128i v,
	const size_t e)
{
	switch (e)
	{
	case 0:
		return _mm_cvtsi128_si32(v);
	case 1:
		return _mm_cvtsi128_si32(_mm_shuffle_epi32(v, 0xe5));
	case 2:
		return _mm_cvtsi128_si32(_mm_shuffle_epi32(v, 0xe6));
	case 3:
		return _mm_cvtsi128_si32(_mm_shuffle_epi32(v, 0xe7));
	};

	assert(0);
	return 0;
}

inline static __m128 set(
	const __m128 v,
	const size_t e,
	const float f)
{
#if __SSE4_1__ == 1
	const __m128 sf = _mm_set1_ps(f);

	switch (e)
	{
	case 0:
		return _mm_move_ss(v, sf);
	case 1:
		return _mm_insert_ps(v, sf, 0x10);
	case 2:
		return _mm_insert_ps(v, sf, 0x20);
	case 3:
		return _mm_insert_ps(v, sf, 0x30);
	}

#else // __SSE4_1__ == 1
#if VECTBASE_SSE2_ALT_SET == 1
	const __m128 sf = _mm_set_ss(f);
	__m128 mask;

	switch (e)
	{
	case 0:
		return _mm_move_ss(v, sf);
	case 1:
		mask = _mm_castsi128_ps(_mm_set_epi32(-1, -1, 0, -1));
		return _mm_or_ps(_mm_shuffle_ps(sf, sf, 0x51), _mm_and_ps(mask, v));
	case 2:
		mask = _mm_castsi128_ps(_mm_set_epi32(-1, 0, -1, -1));
		return _mm_or_ps(_mm_shuffle_ps(sf, sf, 0x45), _mm_and_ps(mask, v));
	case 3:
		mask = _mm_castsi128_ps(_mm_set_epi32(0, -1, -1, -1));
		return _mm_or_ps(_mm_shuffle_ps(sf, sf, 0x15), _mm_and_ps(mask, v));
	}

#else // VECTBASE_SSE2_ALT_SET == 1
	const __m128 sf = _mm_set1_ps(f);

	switch (e)
	{
	case 0:
		return _mm_move_ss(v, sf);
	case 1:
		return _mm_shuffle_ps(_mm_unpacklo_ps(v, sf), v, 0xe4);
	case 2:
		return _mm_shuffle_ps(v, _mm_unpackhi_ps(sf, v), 0xe4);
	case 3:
		return _mm_shuffle_ps(v, _mm_unpackhi_ps(v, sf), 0x44);
	}

#endif // VECTBASE_SSE2_ALT_SET == 1
#endif // __SSE4_1__ == 1
	assert(0);
	return _mm_set1_ps(0);
}

inline static __m128i set(
	const __m128i v,
	const size_t e,
	const int32_t i)
{
#if __SSE4_1__ == 1
	switch (e)
	{
	case 0:
		return _mm_insert_epi32(v, i, 0);
	case 1:
		return _mm_insert_epi32(v, i, 1);
	case 2:
		return _mm_insert_epi32(v, i, 2);
	case 3:
		return _mm_insert_epi32(v, i, 3);
	}

#else // __SSE4_1__ == 1
	const __m128i si = _mm_cvtsi32_si128(i);
	__m128i mask;

	switch (e)
	{
	case 0:
		mask = _mm_set_epi32(-1, -1, -1, 0);
		return _mm_or_si128(si, _mm_and_si128(mask, v));
	case 1:
		mask = _mm_set_epi32(-1, -1, 0, -1);
		return _mm_or_si128(_mm_shuffle_epi32(si, 0x51), _mm_and_si128(mask, v));
	case 2:
		mask = _mm_set_epi32(-1, 0, -1, -1);
		return _mm_or_si128(_mm_shuffle_epi32(si, 0x45), _mm_and_si128(mask, v));
	case 3:
		mask = _mm_set_epi32(0, -1, -1, -1);
		return _mm_or_si128(_mm_shuffle_epi32(si, 0x15), _mm_and_si128(mask, v));
	}

#endif // __SSE4_1__ == 1
	assert(0);
	return _mm_set1_epi32(0);
}

//
// partial specialization for SCALAR_T = float, NATIVE_T = __m128
//

template < size_t DIMENSION >
class vect< float, DIMENSION, __m128 > {
public:

	typedef float scalar_t;
	typedef __m128 native_t;
	typedef int32_t bitmask;

	enum {
		dimension = DIMENSION,
		native_count = (sizeof(scalar_t) * DIMENSION + sizeof(native_t) - 1) / sizeof(native_t),
		native_dimension = native_count * sizeof(native_t) / sizeof(scalar_t),
		scalars_per_native = sizeof(native_t) / sizeof(scalar_t),
		full_mask = ~(bitmask(-1) << scalars_per_native),
		tail_mask = ~(bitmask(-1) << dimension % scalars_per_native),
	};

	vect() {
	}

	// element mutator
	void set(
		const size_t i,
		const float c);

	// element accessor
	float get(
		const size_t i) const;

	// element accessor, array subscript
	float operator [](
		const size_t i) const;

	bool operator ==(
		const vect& src) const;

	bool operator !=(
		const vect& src) const;

	bool operator >(
		const vect& src) const;

	bool operator >=(
		const vect& src) const;

	// native vector mutator
	vect& setn(
		const size_t i,
		const __m128 src);

	// native vector accessor
	__m128 getn(
		const size_t i = 0) const;

protected:

	// native element mutator
	void set_native(
		const size_t i,
		const float c);

private:

#if VECTBASE_MINIMISE_ALIASING == 1
	__m128 n[native_count];

#else
	union {
		__m128 n[native_count];
		float  m[native_dimension];
	};

#endif
};


template < size_t DIMENSION >
inline void
vect< float, DIMENSION, __m128 >::set(
	const size_t i,
	const float c)
{
	assert(i < dimension);

#if VECTBASE_MINIMISE_ALIASING == 1
	n[i / scalars_per_native] = base::set(n[i / scalars_per_native], i % scalars_per_native, c);

#else
	m[i] = c;

#endif
}


template < size_t DIMENSION >
inline void
vect< float, DIMENSION, __m128 >::set_native(
	const size_t i,
	const float c)
{
	assert(i < native_dimension);

#if VECTBASE_MINIMISE_ALIASING == 1
	n[i / scalars_per_native] = base::set(n[i / scalars_per_native], i % scalars_per_native, c);

#else
	m[i] = c;

#endif
}


template < size_t DIMENSION >
inline float
vect< float, DIMENSION, __m128 >::get(
	const size_t i) const
{
	assert(i < dimension);

#if VECTBASE_MINIMISE_ALIASING == 1
	return base::get(n[i / scalars_per_native], i % scalars_per_native);

#else
	return m[i];

#endif
}


template < size_t DIMENSION >
inline float
vect< float, DIMENSION, __m128 >::operator [](
	const size_t i) const
{
	return this->get(i);
}


template < size_t DIMENSION >
inline vect< float, DIMENSION, __m128 >&
vect< float, DIMENSION, __m128 >::setn(
	const size_t i,
	const __m128 src)
{
	assert(i < native_count);

	n[i] = src;

	return *this;
}


template < size_t DIMENSION >
inline __m128
vect< float, DIMENSION, __m128 >::getn(
	const size_t i) const
{
	assert(i < native_count);

	return n[i];
}


template < size_t DIMENSION >
inline bool
vect< float, DIMENSION, __m128 >::operator ==(
	const vect< float, DIMENSION, __m128 >& src) const
{
	for (size_t i = 0; i < dimension / scalars_per_native; ++i)
		if (full_mask != _mm_movemask_ps(_mm_cmpeq_ps(n[i], src.n[i])))
			return false;

	if (dimension == native_dimension)
		return true;

	if (tail_mask != (tail_mask & _mm_movemask_ps(_mm_cmpeq_ps(n[native_count - 1], src.n[native_count - 1]))))
		return false;

	return true;
}


template < size_t DIMENSION >
inline bool
vect< float, DIMENSION, __m128 >::operator !=(
	const vect< float, DIMENSION, __m128 >& src) const
{
	for (size_t i = 0; i < dimension / scalars_per_native; ++i)
		if (full_mask != _mm_movemask_ps(_mm_cmpeq_ps(n[i], src.n[i])))
			return true;

	if (dimension == native_dimension)
		return false;

	if (tail_mask != (tail_mask & _mm_movemask_ps(_mm_cmpeq_ps(n[native_count - 1], src.n[native_count - 1]))))
		return true;

	return false;
}


template < size_t DIMENSION >
inline bool
vect< float, DIMENSION, __m128 >::operator >(
	const vect< float, DIMENSION, __m128 >& src) const
{
	for (size_t i = 0; i < dimension / scalars_per_native; ++i)
		if (full_mask != _mm_movemask_ps(_mm_cmpgt_ps(n[i], src.n[i])))
			return false;

	if (dimension == native_dimension)
		return true;

	if (tail_mask != (tail_mask & _mm_movemask_ps(_mm_cmpgt_ps(n[native_count - 1], src.n[native_count - 1]))))
		return false;

	return true;
}


template < size_t DIMENSION >
inline bool
vect< float, DIMENSION, __m128 >::operator >=(
	const vect< float, DIMENSION, __m128 >& src) const
{
	for (size_t i = 0; i < dimension / scalars_per_native; ++i)
		if (full_mask != _mm_movemask_ps(_mm_cmpge_ps(n[i], src.n[i])))
			return false;

	if (dimension == native_dimension)
		return true;

	if (tail_mask != (tail_mask & _mm_movemask_ps(_mm_cmpge_ps(n[native_count - 1], src.n[native_count - 1]))))
		return false;

	return true;
}

//
// partial specialization for SCALAR_T = int32_t, NATIVE_T = __m128i
//

template < size_t DIMENSION >
class vect< int32_t, DIMENSION, __m128i > {
public:

	typedef int32_t scalar_t;
	typedef __m128i native_t;
	typedef int32_t bitmask;

	enum {
		dimension = DIMENSION,
		native_count = (sizeof(scalar_t) * DIMENSION + sizeof(native_t) - 1) / sizeof(native_t),
		native_dimension = native_count * sizeof(native_t) / sizeof(scalar_t),
		scalars_per_native = sizeof(native_t) / sizeof(scalar_t),
		full_mask = ~(bitmask(-1) << scalars_per_native * 4),
		tail_mask = ~(bitmask(-1) << dimension % scalars_per_native * 4)
	};

	vect() {
	}

	// element mutator
	void set(
		const size_t i,
		const int32_t c);

	// element accessor
	int32_t get(
		const size_t i) const;

	// element accessor, array subscript
	int32_t operator [](
		const size_t i) const;

	bool operator ==(
		const vect& src) const;

	bool operator !=(
		const vect& src) const;

	bool operator >(
		const vect& src) const;

	bool operator >=(
		const vect& src) const;

	// native vector mutator
	vect& setn(
		const size_t i,
		const __m128i src);

	// native vector accessor
	__m128i getn(
		const size_t i = 0) const;

protected:

	// native element mutator
	void set_native(
		const size_t i,
		const int32_t c);

private:

#if VECTBASE_MINIMISE_ALIASING == 1
	__m128i n[native_count];

#else
	union {
		__m128i n[native_count];
		int32_t m[native_dimension];
	};

#endif
};


template < size_t DIMENSION >
inline void
vect< int32_t, DIMENSION, __m128i >::set(
	const size_t i,
	const int32_t c)
{
	assert(i < dimension);

#if VECTBASE_MINIMISE_ALIASING == 1
	n[i / scalars_per_native] = base::set(n[i / scalars_per_native], i % scalars_per_native, c);

#else
	m[i] = c;

#endif
}


template < size_t DIMENSION >
inline void
vect< int32_t, DIMENSION, __m128i >::set_native(
	const size_t i,
	const int32_t c)
{
	assert(i < native_dimension);

#if VECTBASE_MINIMISE_ALIASING == 1
	n[i / scalars_per_native] = base::set(n[i / scalars_per_native], i % scalars_per_native, c);

#else
	m[i] = c;

#endif
}


template < size_t DIMENSION >
inline int32_t
vect< int32_t, DIMENSION, __m128i >::get(
	const size_t i) const
{
	assert(i < dimension);

#if VECTBASE_MINIMISE_ALIASING == 1
	return base::get(n[i / scalars_per_native], i % scalars_per_native);

#else
	return m[i];

#endif
}


template < size_t DIMENSION >
inline int32_t
vect< int32_t, DIMENSION, __m128i >::operator [](
	const size_t i) const
{
	return this->get(i);
}


template < size_t DIMENSION >
inline vect< int32_t, DIMENSION, __m128i >&
vect< int32_t, DIMENSION, __m128i >::setn(
	const size_t i,
	const __m128i src)
{
	assert(i < native_count);

	n[i] = src;

	return *this;
}


template < size_t DIMENSION >
inline __m128i
vect< int32_t, DIMENSION, __m128i >::getn(
	const size_t i) const
{
	assert(i < native_count);

	return n[i];
}


template < size_t DIMENSION >
inline bool
vect< int32_t, DIMENSION, __m128i >::operator ==(
	const vect< int32_t, DIMENSION, __m128i >& src) const
{
	for (size_t i = 0; i < dimension / scalars_per_native; ++i)
		if (full_mask != _mm_movemask_epi8(_mm_cmpeq_epi32(n[i], src.n[i])))
			return false;

	if (dimension == native_dimension)
		return true;

	if (tail_mask != (tail_mask &_mm_movemask_epi8(_mm_cmpeq_epi32(n[native_count - 1], src.n[native_count - 1]))))
		return false;

	return true;
}


template < size_t DIMENSION >
inline bool
vect< int32_t, DIMENSION, __m128i >::operator !=(
	const vect< int32_t, DIMENSION, __m128i >& src) const
{
	for (size_t i = 0; i < dimension / scalars_per_native; ++i)
		if (full_mask != _mm_movemask_epi8(_mm_cmpeq_epi32(n[i], src.n[i])))
			return true;

	if (dimension == native_dimension)
		return false;

	if (tail_mask != (tail_mask & _mm_movemask_epi8(_mm_cmpeq_epi32(n[native_count - 1], src.n[native_count - 1]))))
		return true;

	return false;
}


template < size_t DIMENSION >
inline bool
vect< int32_t, DIMENSION, __m128i >::operator >(
	const vect< int32_t, DIMENSION, __m128i >& src) const
{
	for (size_t i = 0; i < dimension / scalars_per_native; ++i)
		if (full_mask != _mm_movemask_epi8(_mm_cmpgt_epi32(n[i], src.n[i])))
			return false;

	if (dimension == native_dimension)
		return true;

	if (tail_mask != (tail_mask &_mm_movemask_epi8(_mm_cmpgt_epi32(n[native_count - 1], src.n[native_count - 1]))))
		return false;

	return true;
}


template < size_t DIMENSION >
inline bool
vect< int32_t, DIMENSION, __m128i >::operator >=(
	const vect< int32_t, DIMENSION, __m128i >& src) const
{
	for (size_t i = 0; i < dimension / scalars_per_native; ++i)
		if (full_mask != _mm_movemask_epi8(_mm_cmpge_epi32(n[i], src.n[i])))
			return false;

	if (dimension == native_dimension)
		return true;

	if (tail_mask != (tail_mask &_mm_movemask_epi8(_mm_cmpge_epi32(n[native_count - 1], src.n[native_count - 1]))))
		return false;

	return true;
}

} // namespace base

#endif // vect_base_sse_H__
