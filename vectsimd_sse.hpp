#ifndef vect_simd_sse_H__
#define vect_simd_sse_H__

#include <stdint.h>
#include <cmath>
#include <cassert>
#include "vectbase_sse.hpp"

namespace simd
{

////////////////////////////////////////////////////////////////////////////////////////////////////
// class protovect
// type- and dimensionality-agnostic vector; provides seamless downcasting and a basic set of vector
// ops; subclass as:
//
//		class myVect : public protovect< myVect, myScalarType, myDimension, myNative >
//
////////////////////////////////////////////////////////////////////////////////////////////////////

template < typename SUBCLASS_T, typename SCALAR_T, size_t DIMENSION, typename NATIVE_T >
class protovect : public base::vect< SCALAR_T, DIMENSION, NATIVE_T >
{
public:

	typedef base::vect< SCALAR_T, DIMENSION, NATIVE_T > basetype;
	typedef SUBCLASS_T subclass;

	protovect() {
	}

	explicit protovect(
		const base::vect< SCALAR_T, DIMENSION, NATIVE_T >& src)
	: base::vect< SCALAR_T, DIMENSION, NATIVE_T >(src) {
	}

	explicit protovect(
		const SCALAR_T (& src)[DIMENSION],
		const bool zero_first = false);

	operator SUBCLASS_T&();

	operator const SUBCLASS_T&() const;

	// negative of argument
	SUBCLASS_T& negate(
		const base::vect< SCALAR_T, DIMENSION, NATIVE_T >& src);

	// negative of this
	SUBCLASS_T& negate();

	// product of argument and scalar argument
	SUBCLASS_T& mul(
		const base::vect< SCALAR_T, DIMENSION, NATIVE_T >& src,
		const SCALAR_T c);

	// product of this and scalar argument
	SUBCLASS_T& mul(
		const SCALAR_T c);

	// sum of arguments
	SUBCLASS_T& add(
		const base::vect< SCALAR_T, DIMENSION, NATIVE_T >& src0,
		const base::vect< SCALAR_T, DIMENSION, NATIVE_T >& src1);

	// sum of this and argument
	SUBCLASS_T& add(
		const base::vect< SCALAR_T, DIMENSION, NATIVE_T >& src);

	// difference between arguments
	SUBCLASS_T& sub(
		const base::vect< SCALAR_T, DIMENSION, NATIVE_T >& src0,
		const base::vect< SCALAR_T, DIMENSION, NATIVE_T >& src1);

	// difference between this and argument
	SUBCLASS_T& subr(
		const base::vect< SCALAR_T, DIMENSION, NATIVE_T >& src);

	// difference between argument and this
	SUBCLASS_T& subl(
		const base::vect< SCALAR_T, DIMENSION, NATIVE_T >& src);

	// product of arguments
	SUBCLASS_T& mul(
		const base::vect< SCALAR_T, DIMENSION, NATIVE_T >& src0,
		const base::vect< SCALAR_T, DIMENSION, NATIVE_T >& src1);

	// product of this and argument
	SUBCLASS_T& mul(
		const base::vect< SCALAR_T, DIMENSION, NATIVE_T >& src);

	// product of first two arguments, added to third argument
	SUBCLASS_T& mad(
		const base::vect< SCALAR_T, DIMENSION, NATIVE_T >& src0,
		const base::vect< SCALAR_T, DIMENSION, NATIVE_T >& src1,
		const base::vect< SCALAR_T, DIMENSION, NATIVE_T >& src2);

	// product of arguments, added to this
	SUBCLASS_T& mad(
		const base::vect< SCALAR_T, DIMENSION, NATIVE_T >& src0,
		const base::vect< SCALAR_T, DIMENSION, NATIVE_T >& src1);

	// product of argument and scalar, added to last argument
	SUBCLASS_T& mad(
		const base::vect< SCALAR_T, DIMENSION, NATIVE_T >& src0,
		const SCALAR_T c,
		const base::vect< SCALAR_T, DIMENSION, NATIVE_T >& src1);

	// product of argument and scalar, added to this
	SUBCLASS_T& mad(
		const base::vect< SCALAR_T, DIMENSION, NATIVE_T >& src,
		const SCALAR_T c);

	// division of first by second argument
	SUBCLASS_T& div(
		const base::vect< SCALAR_T, DIMENSION, NATIVE_T >& src0,
		const base::vect< SCALAR_T, DIMENSION, NATIVE_T >& src1);

	// division of this by argument
	SUBCLASS_T& divr(
		const base::vect< SCALAR_T, DIMENSION, NATIVE_T >& src);

	// division of argument by this
	SUBCLASS_T& divl(
		const base::vect< SCALAR_T, DIMENSION, NATIVE_T >& src);

	// weighted sum of arguments
	SUBCLASS_T& wsum(
		const base::vect< SCALAR_T, DIMENSION, NATIVE_T >& src0,
		const base::vect< SCALAR_T, DIMENSION, NATIVE_T >& src1,
		const SCALAR_T factor0,
		const SCALAR_T factor1);

	// weighted sum of this and argument
	SUBCLASS_T& wsum(
		const base::vect< SCALAR_T, DIMENSION, NATIVE_T >& src,
		const SCALAR_T factor0,
		const SCALAR_T factor1);

	// set this to zeros
	SUBCLASS_T& zero();
};


template < typename SUBCLASS_T, typename SCALAR_T, size_t DIMENSION, typename NATIVE_T >
inline
protovect< SUBCLASS_T, SCALAR_T, DIMENSION, NATIVE_T >::operator SUBCLASS_T&()
{
	return *static_cast< SUBCLASS_T* >(this);
}


template < typename SUBCLASS_T, typename SCALAR_T, size_t DIMENSION, typename NATIVE_T >
inline
protovect< SUBCLASS_T, SCALAR_T, DIMENSION, NATIVE_T >::operator const SUBCLASS_T&() const
{
	return *static_cast< const SUBCLASS_T* >(this);
}


template < typename SUBCLASS_T, typename SCALAR_T, size_t DIMENSION, typename NATIVE_T >
inline protovect< SUBCLASS_T, SCALAR_T, DIMENSION, NATIVE_T >::protovect(
	const SCALAR_T (& src)[DIMENSION],
	const bool zero_first)
{
	const bool zero_init = DIMENSION != this->native_dimension && zero_first;

	SUBCLASS_T* const subthis = static_cast< SUBCLASS_T* >(this);

	if (zero_init)
		subthis->zero();

	for (size_t i = 0; i < DIMENSION; ++i)
		subthis->set(i, src[i]);

	if (zero_init)
		return;

	for (size_t i = DIMENSION; i < this->native_dimension; ++i)
		subthis->set_native(i, src[DIMENSION - 1]);
}


template < typename SUBCLASS_T, typename SCALAR_T, size_t DIMENSION, typename NATIVE_T >
inline SUBCLASS_T&
protovect< SUBCLASS_T, SCALAR_T, DIMENSION, NATIVE_T >::mul(
	const base::vect< SCALAR_T, DIMENSION, NATIVE_T >& src,
	const SCALAR_T c)
{
	SUBCLASS_T* const subthis = static_cast< SUBCLASS_T* >(this);

	for (size_t i = 0; i < DIMENSION; ++i)
		subthis->set(i, src[i] * c);

	return *this;
}


template < typename SUBCLASS_T, typename SCALAR_T, size_t DIMENSION, typename NATIVE_T >
inline SUBCLASS_T&
protovect< SUBCLASS_T, SCALAR_T, DIMENSION, NATIVE_T >::mul(
	const SCALAR_T c)
{
	return *this = SUBCLASS_T().mul(*this, c);
}


template < typename SUBCLASS_T, typename SCALAR_T, size_t DIMENSION, typename NATIVE_T >
inline SUBCLASS_T&
protovect< SUBCLASS_T, SCALAR_T, DIMENSION, NATIVE_T >::negate(
	const base::vect< SCALAR_T, DIMENSION, NATIVE_T >& src)
{
	SUBCLASS_T* const subthis = static_cast< SUBCLASS_T* >(this);

	for (size_t i = 0; i < DIMENSION; ++i)
		subthis->set(i, -src[i]);

	return *this;
}


template < typename SUBCLASS_T, typename SCALAR_T, size_t DIMENSION, typename NATIVE_T >
inline SUBCLASS_T&
protovect< SUBCLASS_T, SCALAR_T, DIMENSION, NATIVE_T >::negate()
{
	return *this = SUBCLASS_T().negate(*this);
}


template < typename SUBCLASS_T, typename SCALAR_T, size_t DIMENSION, typename NATIVE_T >
inline SUBCLASS_T&
protovect< SUBCLASS_T, SCALAR_T, DIMENSION, NATIVE_T >::add(
	const base::vect< SCALAR_T, DIMENSION, NATIVE_T >& src0,
	const base::vect< SCALAR_T, DIMENSION, NATIVE_T >& src1)
{
	SUBCLASS_T* const subthis = static_cast< SUBCLASS_T* >(this);

	for (size_t i = 0; i < DIMENSION; ++i)
		subthis->set(i, src0[i] + src1[i]);

	return *this;
}


template < typename SUBCLASS_T, typename SCALAR_T, size_t DIMENSION, typename NATIVE_T >
inline SUBCLASS_T&
protovect< SUBCLASS_T, SCALAR_T, DIMENSION, NATIVE_T >::add(
	const base::vect< SCALAR_T, DIMENSION, NATIVE_T >& src)
{
	return *this = SUBCLASS_T().add(*this, src);
}


template < typename SUBCLASS_T, typename SCALAR_T, size_t DIMENSION, typename NATIVE_T >
inline SUBCLASS_T&
protovect< SUBCLASS_T, SCALAR_T, DIMENSION, NATIVE_T >::sub(
	const base::vect< SCALAR_T, DIMENSION, NATIVE_T >& src0,
	const base::vect< SCALAR_T, DIMENSION, NATIVE_T >& src1)
{
	SUBCLASS_T* const subthis = static_cast< SUBCLASS_T* >(this);

	for (size_t i = 0; i < DIMENSION; ++i)
		subthis->set(i, src0[i] - src1[i]);

	return *this;
}


template < typename SUBCLASS_T, typename SCALAR_T, size_t DIMENSION, typename NATIVE_T >
inline SUBCLASS_T&
protovect< SUBCLASS_T, SCALAR_T, DIMENSION, NATIVE_T >::subr(
	const base::vect< SCALAR_T, DIMENSION, NATIVE_T >& src)
{
	return *this = SUBCLASS_T().sub(*this, src);
}


template < typename SUBCLASS_T, typename SCALAR_T, size_t DIMENSION, typename NATIVE_T >
inline SUBCLASS_T&
protovect< SUBCLASS_T, SCALAR_T, DIMENSION, NATIVE_T >::subl(
	const base::vect< SCALAR_T, DIMENSION, NATIVE_T >& src)
{
	return *this = SUBCLASS_T().sub(src, *this);
}


template < typename SUBCLASS_T, typename SCALAR_T, size_t DIMENSION, typename NATIVE_T >
inline SUBCLASS_T&
protovect< SUBCLASS_T, SCALAR_T, DIMENSION, NATIVE_T >::mul(
	const base::vect< SCALAR_T, DIMENSION, NATIVE_T >& src0,
	const base::vect< SCALAR_T, DIMENSION, NATIVE_T >& src1)
{
	SUBCLASS_T* const subthis = static_cast< SUBCLASS_T* >(this);

	for (size_t i = 0; i < DIMENSION; ++i)
		subthis->set(i, src0[i] * src1[i]);

	return *this;
}


template < typename SUBCLASS_T, typename SCALAR_T, size_t DIMENSION, typename NATIVE_T >
inline SUBCLASS_T&
protovect< SUBCLASS_T, SCALAR_T, DIMENSION, NATIVE_T >::mul(
	const base::vect< SCALAR_T, DIMENSION, NATIVE_T >& src)
{
	return *this = SUBCLASS_T().mul(*this, src);
}


template < typename SUBCLASS_T, typename SCALAR_T, size_t DIMENSION, typename NATIVE_T >
inline SUBCLASS_T&
protovect< SUBCLASS_T, SCALAR_T, DIMENSION, NATIVE_T >::mad(
	const base::vect< SCALAR_T, DIMENSION, NATIVE_T >& src0,
	const base::vect< SCALAR_T, DIMENSION, NATIVE_T >& src1,
	const base::vect< SCALAR_T, DIMENSION, NATIVE_T >& src2)
{
	SUBCLASS_T* const subthis = static_cast< SUBCLASS_T* >(this);

	for (size_t i = 0; i < DIMENSION; ++i)
		subthis->set(i, src0[i] * src1[i] + src2[i]);

	return *this;
}


template < typename SUBCLASS_T, typename SCALAR_T, size_t DIMENSION, typename NATIVE_T >
inline SUBCLASS_T&
protovect< SUBCLASS_T, SCALAR_T, DIMENSION, NATIVE_T >::mad(
	const base::vect< SCALAR_T, DIMENSION, NATIVE_T >& src0,
	const base::vect< SCALAR_T, DIMENSION, NATIVE_T >& src1)
{
	return *this = SUBCLASS_T().mad(src0, src1, *this);
}


template < typename SUBCLASS_T, typename SCALAR_T, size_t DIMENSION, typename NATIVE_T >
inline SUBCLASS_T&
protovect< SUBCLASS_T, SCALAR_T, DIMENSION, NATIVE_T >::mad(
	const base::vect< SCALAR_T, DIMENSION, NATIVE_T >& src0,
	const SCALAR_T c,
	const base::vect< SCALAR_T, DIMENSION, NATIVE_T >& src1)
{
	SUBCLASS_T* const subthis = static_cast< SUBCLASS_T* >(this);

	for (size_t i = 0; i < DIMENSION; ++i)
		subthis->set(i, src0[i] * c + src1[i]);

	return *this;
}


template < typename SUBCLASS_T, typename SCALAR_T, size_t DIMENSION, typename NATIVE_T >
inline SUBCLASS_T&
protovect< SUBCLASS_T, SCALAR_T, DIMENSION, NATIVE_T >::mad(
	const base::vect< SCALAR_T, DIMENSION, NATIVE_T >& src,
	const SCALAR_T c)
{
	return *this = SUBCLASS_T().mad(src, c, *this);
}


template < typename SUBCLASS_T, typename SCALAR_T, size_t DIMENSION, typename NATIVE_T >
inline SUBCLASS_T&
protovect< SUBCLASS_T, SCALAR_T, DIMENSION, NATIVE_T >::div(
	const base::vect< SCALAR_T, DIMENSION, NATIVE_T >& src0,
	const base::vect< SCALAR_T, DIMENSION, NATIVE_T >& src1)
{
	SUBCLASS_T* const subthis = static_cast< SUBCLASS_T* >(this);

	for (size_t i = 0; i < DIMENSION; ++i)
		subthis->set(i, src0[i] / src1[i]);

	return *this;
}


template < typename SUBCLASS_T, typename SCALAR_T, size_t DIMENSION, typename NATIVE_T >
inline SUBCLASS_T&
protovect< SUBCLASS_T, SCALAR_T, DIMENSION, NATIVE_T >::divr(
	const base::vect< SCALAR_T, DIMENSION, NATIVE_T >& src)
{
	return *this = SUBCLASS_T().div(*this, src);
}


template < typename SUBCLASS_T, typename SCALAR_T, size_t DIMENSION, typename NATIVE_T >
inline SUBCLASS_T&
protovect< SUBCLASS_T, SCALAR_T, DIMENSION, NATIVE_T >::divl(
	const base::vect< SCALAR_T, DIMENSION, NATIVE_T >& src)
{
	return *this = SUBCLASS_T().div(src, *this);
}


template < typename SUBCLASS_T, typename SCALAR_T, size_t DIMENSION, typename NATIVE_T >
inline SUBCLASS_T&
protovect< SUBCLASS_T, SCALAR_T, DIMENSION, NATIVE_T >::wsum(
	const base::vect< SCALAR_T, DIMENSION, NATIVE_T >& src0,
	const base::vect< SCALAR_T, DIMENSION, NATIVE_T >& src1,
	const SCALAR_T factor0,
	const SCALAR_T factor1)
{
	SUBCLASS_T* const subthis = static_cast< SUBCLASS_T* >(this);

	for (size_t i = 0; i < DIMENSION; ++i)
		subthis->set(i, src0[i] * factor0 + src1[i] * factor1);

	return *this;
}


template < typename SUBCLASS_T, typename SCALAR_T, size_t DIMENSION, typename NATIVE_T >
inline SUBCLASS_T&
protovect< SUBCLASS_T, SCALAR_T, DIMENSION, NATIVE_T >::wsum(
	const base::vect< SCALAR_T, DIMENSION, NATIVE_T >& src,
	const SCALAR_T factor0,
	const SCALAR_T factor1)
{
	return *this = SUBCLASS_T().wsum(*this, src, factor0, factor1);
}


template < typename SUBCLASS_T, typename SCALAR_T, size_t DIMENSION, typename NATIVE_T >
inline SUBCLASS_T&
protovect< SUBCLASS_T, SCALAR_T, DIMENSION, NATIVE_T >::zero()
{
	SUBCLASS_T* const subthis = static_cast< SUBCLASS_T* >(this);

	for (size_t i = 0; i < this->native_dimension; ++i)
		subthis->set_native(i, 0.f);

	return *this;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// class vect_generic
// minimalist instantiatable specialization of protovect; chances are you want to use something else
// as one should normally aim for the most derived subclass that fits a scenario
////////////////////////////////////////////////////////////////////////////////////////////////////

template < typename SCALAR_T, size_t DIMENSION, typename NATIVE_T >
class vect_generic : public protovect<
	vect_generic< SCALAR_T, DIMENSION, NATIVE_T >, SCALAR_T, DIMENSION, NATIVE_T >
{
public:

	vect_generic() {
	}

	explicit vect_generic(
		const base::vect< SCALAR_T, DIMENSION, NATIVE_T >& src)
	: protovect< vect_generic, SCALAR_T, DIMENSION, NATIVE_T >(src) {
	}

	explicit vect_generic(
		const SCALAR_T (& src)[DIMENSION],
		const bool zero_first = false);
};


template < typename SCALAR_T, size_t DIMENSION, typename NATIVE_T >
inline vect_generic< SCALAR_T, DIMENSION, NATIVE_T >::vect_generic(
	const SCALAR_T (& src)[DIMENSION],
	const bool zero_first)
: protovect< vect_generic, SCALAR_T, DIMENSION, NATIVE_T >(src, zero_first)
{
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// class vectz
// integer-based dimensionality-agnostic vector; hosts integer-only functionality
////////////////////////////////////////////////////////////////////////////////////////////////////

template < typename SUBCLASS_T, size_t DIMENSION, typename NATIVE_T >
class vectz : public protovect< SUBCLASS_T, int32_t, DIMENSION, NATIVE_T >
{
public:

	vectz() {
	}

	explicit vectz(
		const base::vect< int32_t, DIMENSION, NATIVE_T >& src)
	: protovect< SUBCLASS_T, int32_t, DIMENSION, NATIVE_T >(src) {
	}

	explicit vectz(
		const int (& src)[DIMENSION],
		const bool zero_first = false);

	operator SUBCLASS_T&();

	operator const SUBCLASS_T&() const;
};


template < typename SUBCLASS_T, size_t DIMENSION, typename NATIVE_T >
inline vectz< SUBCLASS_T, DIMENSION, NATIVE_T >::vectz(
	const int (& src)[DIMENSION],
	const bool zero_first)
: protovect< SUBCLASS_T, int32_t, DIMENSION, NATIVE_T >(src, zero_first)
{
}


template < typename SUBCLASS_T, size_t DIMENSION, typename NATIVE_T >
inline vectz< SUBCLASS_T, DIMENSION, NATIVE_T >::operator SUBCLASS_T&()
{
	return *static_cast< SUBCLASS_T* >(this);
}


template < typename SUBCLASS_T, size_t DIMENSION, typename NATIVE_T >
inline vectz< SUBCLASS_T, DIMENSION, NATIVE_T >::operator const SUBCLASS_T&() const
{
	return *static_cast< const SUBCLASS_T* >(this);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// class ivect
// convenience wrapper of class vectz
////////////////////////////////////////////////////////////////////////////////////////////////////

template < size_t DIMENSION, typename NATIVE_T >
class ivect : public vectz< ivect< DIMENSION, NATIVE_T >, DIMENSION, NATIVE_T >
{
public:

	ivect() {
	}

	explicit ivect(
		const base::vect< int32_t, DIMENSION, NATIVE_T >& src)
	: vectz< ivect, DIMENSION, NATIVE_T >(src) {
	}

	explicit ivect(
		const int (& src)[DIMENSION],
		const bool zero_first = false);
};


template < size_t DIMENSION, typename NATIVE_T >
inline ivect< DIMENSION, NATIVE_T >::ivect(
	const int (& src)[DIMENSION],
	const bool zero_first)
: vectz< ivect, DIMENSION, NATIVE_T >(src, zero_first)
{
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// class ivect<2>
// specialization of ivect for DIMENSION = 2, NATIVE_T = __m128i
////////////////////////////////////////////////////////////////////////////////////////////////////

template <>
class ivect< 2, __m128i > : public vectz< ivect< 2, __m128i >, 2, __m128i >
{
public:

	ivect() {
	}

	explicit ivect(
		const base::vect< int32_t, 2, __m128i >& src)
	: vectz< ivect, 2, __m128i >(src) {
	}

	explicit ivect(
		const int (& src)[2],
		const bool zero_first = false);

	explicit ivect(
		const int c0,
		const int c1,
		const bool zero_first = false);
};


inline ivect< 2, __m128i >::ivect(
	const int (& src)[2],
	const bool zero_first)
: vectz< ivect, 2, __m128i >(src, zero_first)
{
}


inline ivect< 2, __m128i >::ivect(
	const int c0,
	const int c1,
	const bool zero_first)
{
	const bool zero_init = this->dimension != this->native_dimension && zero_first;

	if (zero_init)
		this->zero();

	this->set(0, c0);
	this->set(1, c1);

	if (zero_init)
		return;

	for (size_t i = this->dimension; i < this->native_dimension; ++i)
		this->set_native(i, c1);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// class ivect<3>
// specialization of ivect for DIMENSION = 3, NATIVE_T = __m128i
////////////////////////////////////////////////////////////////////////////////////////////////////

template <>
class ivect< 3, __m128i > : public vectz< ivect< 3, __m128i >, 3, __m128i >
{
public:

	ivect() {
	}

	explicit ivect(
		const base::vect< int32_t, 3, __m128i >& src)
	: vectz< ivect, 3, __m128i >(src) {
	}

	explicit ivect(
		const int (& src)[3],
		const bool zero_first = false);

	explicit ivect(
		const int c0,
		const int c1,
		const int c2,
		const bool zero_first = false);
};


inline ivect< 3, __m128i >::ivect(
	const int (& src)[3],
	const bool zero_first)
: vectz< ivect, 3, __m128i >(src, zero_first)
{
}


inline ivect< 3, __m128i >::ivect(
	const int c0,
	const int c1,
	const int c2,
	const bool zero_first)
{
	const bool zero_init = this->dimension != this->native_dimension && zero_first;

	if (zero_init)
		this->zero();

	this->set(0, c0);
	this->set(1, c1);
	this->set(2, c2);

	if (zero_init)
		return;

	for (size_t i = this->dimension; i < this->native_dimension; ++i)
		this->set_native(i, c2);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// class ivect<4>
// specialization of ivect for DIMENSION = 4, NATIVE_T = __m128i
////////////////////////////////////////////////////////////////////////////////////////////////////

template <>
class ivect< 4, __m128i > : public vectz< ivect< 4, __m128i >, 4, __m128i >
{
public:

	ivect() {
	}

	explicit ivect(
		const base::vect< int32_t, 4, __m128i >& src)
	: vectz< ivect, 4, __m128i >(src) {
	}

	explicit ivect(
		const int (& src)[4],
		const bool zero_first = false);

	explicit ivect(
		const int c0,
		const int c1,
		const int c2,
		const int c3,
		const bool zero_first = false);
};


inline ivect< 4, __m128i >::ivect(
	const int (& src)[4],
	const bool zero_first)
: vectz< ivect, 4, __m128i >(src, zero_first)
{
}


inline ivect< 4, __m128i >::ivect(
	const int c0,
	const int c1,
	const int c2,
	const int c3,
	const bool)
{
	this->set(0, c0);
	this->set(1, c1);
	this->set(2, c2);
	this->set(3, c3);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// class vectr
// float-based dimensionality-agnostic vector; hosts float-only functionality
////////////////////////////////////////////////////////////////////////////////////////////////////

template < typename SUBCLASS_T, size_t DIMENSION, typename NATIVE_T >
class vectr : public protovect< SUBCLASS_T, float, DIMENSION, NATIVE_T >
{
public:

	vectr() {
	}

	explicit vectr(
		const base::vect< float, DIMENSION, NATIVE_T >& src)
	: protovect< SUBCLASS_T, float, DIMENSION, NATIVE_T >(src) {
	}

	explicit vectr(
		const float (& src)[DIMENSION],
		const bool zero_first = false);

	operator SUBCLASS_T&();

	operator const SUBCLASS_T&() const;

	// dot product
	float dot(
		const base::vect< float, DIMENSION, NATIVE_T >& src) const;

	// Euclidean norm, squared
	float sqr() const;

	// Euclidean norm
	float norm() const;

	// normalise this
	SUBCLASS_T& normalise();

	// normalise argument
	SUBCLASS_T& normalise(
		const base::vect< float, DIMENSION, NATIVE_T >& src);

	// reciprocal of this
	SUBCLASS_T& rcp();

	// reciprocal of argument
	SUBCLASS_T& rcp(
		const base::vect< float, DIMENSION, NATIVE_T >& src);

	// product of argument and row-major matrix operator
	SUBCLASS_T& mul(
		const base::vect< float, DIMENSION, NATIVE_T >& src,
		const base::matx< float, DIMENSION, NATIVE_T >& op);

	// product of this and row-major matrix operator
	SUBCLASS_T& mul(
		const base::matx< float, DIMENSION, NATIVE_T >& op);

	using protovect< SUBCLASS_T, float, DIMENSION, NATIVE_T >::mul;

	// product of argument and row-major matrix operator; last element of argument assumed 1
	SUBCLASS_T& mulH(
		const base::vect< float, DIMENSION, NATIVE_T >& src,
		const base::matx< float, DIMENSION, NATIVE_T >& op);

	// product of this and row-major matrix operator; last element of this assumed 1
	SUBCLASS_T& mulH(
		const base::matx< float, DIMENSION, NATIVE_T >& op);
};


template < typename SUBCLASS_T, size_t DIMENSION, typename NATIVE_T >
inline vectr< SUBCLASS_T, DIMENSION, NATIVE_T >::vectr(
	const float (& src)[DIMENSION],
	const bool zero_first)
: protovect< SUBCLASS_T, float, DIMENSION, NATIVE_T >(src, zero_first)
{
}


template < typename SUBCLASS_T, size_t DIMENSION, typename NATIVE_T >
inline vectr< SUBCLASS_T, DIMENSION, NATIVE_T >::operator SUBCLASS_T&()
{
	return *static_cast< SUBCLASS_T* >(this);
}


template < typename SUBCLASS_T, size_t DIMENSION, typename NATIVE_T >
inline vectr< SUBCLASS_T, DIMENSION, NATIVE_T >::operator const SUBCLASS_T&() const
{
	return *static_cast< const SUBCLASS_T* >(this);
}


template < typename SUBCLASS_T, size_t DIMENSION, typename NATIVE_T >
inline float
vectr< SUBCLASS_T, DIMENSION, NATIVE_T >::dot(
	const base::vect< float, DIMENSION, NATIVE_T >& src) const
{
	float t = this->operator [](0) * src[0];

	for (size_t i = 1; i < DIMENSION; ++i)
		t += this->operator [](i) * src[i];

	return t;
}


template < typename SUBCLASS_T, size_t DIMENSION, typename NATIVE_T >
inline float
vectr< SUBCLASS_T, DIMENSION, NATIVE_T >::sqr() const
{
	const SUBCLASS_T* const subthis = static_cast< const SUBCLASS_T* >(this);

	return subthis->dot(*this);
}


template < typename SUBCLASS_T, size_t DIMENSION, typename NATIVE_T >
inline float
vectr< SUBCLASS_T, DIMENSION, NATIVE_T >::norm() const
{
	const SUBCLASS_T* const subthis = static_cast< const SUBCLASS_T* >(this);

	return sqrt(subthis->sqr());
}


template < typename SUBCLASS_T, size_t DIMENSION, typename NATIVE_T >
inline SUBCLASS_T&
vectr< SUBCLASS_T, DIMENSION, NATIVE_T >::normalise(
	const base::vect< float, DIMENSION, NATIVE_T >& src)
{
	SUBCLASS_T* const subthis = static_cast< SUBCLASS_T* >(this);

	return subthis->mul(src, 1.f / static_cast< const SUBCLASS_T& >(src).norm());
}


template < typename SUBCLASS_T, size_t DIMENSION, typename NATIVE_T >
inline SUBCLASS_T&
vectr< SUBCLASS_T, DIMENSION, NATIVE_T >::normalise()
{
	return *this = SUBCLASS_T().normalise(*this);
}


template < typename SUBCLASS_T, size_t DIMENSION, typename NATIVE_T >
inline SUBCLASS_T&
vectr< SUBCLASS_T, DIMENSION, NATIVE_T >::rcp(
	const base::vect< float, DIMENSION, NATIVE_T >& src)
{
	SUBCLASS_T* const subthis = static_cast< SUBCLASS_T* >(this);

	for (size_t i = 0; i < DIMENSION; ++i)
		subthis->set(i, 1.f / src[i]);

	return *this;
}


template < typename SUBCLASS_T, size_t DIMENSION, typename NATIVE_T >
inline SUBCLASS_T& 
vectr< SUBCLASS_T, DIMENSION, NATIVE_T >::rcp()
{
	return *this = SUBCLASS_T().rcp(*this);
}


template < typename SUBCLASS_T, size_t DIMENSION, typename NATIVE_T >
inline SUBCLASS_T&
vectr< SUBCLASS_T, DIMENSION, NATIVE_T >::mul(
	const base::vect< float, DIMENSION, NATIVE_T >& src,
	const base::matx< float, DIMENSION, NATIVE_T >& op)
{
	SUBCLASS_T* const subthis = static_cast< SUBCLASS_T* >(this);

	subthis->mul(op[0], src[0]);

	for (size_t i = 1; i < DIMENSION; ++i)
		subthis->mad(op[i], src[i]);

	return *this;
}


template < typename SUBCLASS_T, size_t DIMENSION, typename NATIVE_T >
inline SUBCLASS_T&
vectr< SUBCLASS_T, DIMENSION, NATIVE_T >::mul(
	const base::matx< float, DIMENSION, NATIVE_T >& op)
{
	return *this = SUBCLASS_T().mul(*this, op);
}


template < typename SUBCLASS_T, size_t DIMENSION, typename NATIVE_T >
inline SUBCLASS_T&
vectr< SUBCLASS_T, DIMENSION, NATIVE_T >::mulH(
	const base::vect< float, DIMENSION, NATIVE_T >& src,
	const base::matx< float, DIMENSION, NATIVE_T >& op)
{
	assert(1.f == src[DIMENSION - 1]);

	SUBCLASS_T* const subthis = static_cast< SUBCLASS_T* >(this);

	*this = op[DIMENSION - 1];

	for (size_t i = 0; i < DIMENSION - 1; ++i)
		subthis->mad(op[i], src[i]);

	return *this;
}


template < typename SUBCLASS_T, size_t DIMENSION, typename NATIVE_T >
inline SUBCLASS_T&
vectr< SUBCLASS_T, DIMENSION, NATIVE_T >::mulH(
	const base::matx< float, DIMENSION, NATIVE_T >& op)
{
	return *this = SUBCLASS_T().mulH(*this, op);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// class vect
// convenience wrapper of class vectr
////////////////////////////////////////////////////////////////////////////////////////////////////

template < size_t DIMENSION, typename NATIVE_T >
class vect : public vectr< vect< DIMENSION, NATIVE_T >, DIMENSION, NATIVE_T >
{
public:

	vect() {
	}

	explicit vect(
		const base::vect< float, DIMENSION, NATIVE_T >& src)
	: vectr< vect, DIMENSION, NATIVE_T >(src) {
	}

	explicit vect(
		const float (& src)[DIMENSION],
		const bool zero_first = false);
};


template < size_t DIMENSION, typename NATIVE_T >
inline vect< DIMENSION, NATIVE_T >::vect(
	const float (& src)[DIMENSION],
	const bool zero_first)
: vectr< vect, DIMENSION, NATIVE_T >(src, zero_first)
{
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// class vect<2>
// specialization of vect for DIMENSION = 2, NATIVE_T = __m128
////////////////////////////////////////////////////////////////////////////////////////////////////

template <>
class vect< 2, __m128 > : public vectr< vect< 2, __m128 >, 2, __m128 >
{
public:

	vect() {
	}

	explicit vect(
		const base::vect< float, 2, __m128 >& src)
	: vectr< vect, 2, __m128 >(src) {
	}

	explicit vect(
		const float (& src)[2],
		const bool zero_first = false);

	explicit vect(
		const float c0,
		const float c1,
		const bool zero_first = false);

	// cross-product of this and argument, splat across this
	vect& crossr(
		const base::vect< float, 2, __m128 >& src);

	// cross-product of arguments, splat across this
	vect& cross(
		const base::vect< float, 2, __m128 >& src0,
		const base::vect< float, 2, __m128 >& src1);
};


inline vect< 2, __m128 >::vect(
	const float (& src)[2],
	const bool zero_first)
: vectr< vect, 2, __m128 >(src, zero_first)
{
}


inline vect< 2, __m128 >::vect(
	const float c0,
	const float c1,
	const bool zero_first)
{
	const bool zero_init = this->dimension != this->native_dimension && zero_first;

	if (zero_init)
		this->zero();

	this->set(0, c0);
	this->set(1, c1);

	if (zero_init)
		return;

	for (size_t i = this->dimension; i < this->native_dimension; ++i)
		this->set_native(i, c1);
}


inline vect< 2, __m128 >&
vect< 2, __m128 >::cross(
	const base::vect< float, 2, __m128 >& src0,
	const base::vect< float, 2, __m128 >& src1)
{
	const float c = src0[0] * src1[1] - src1[0] * src0[1];

	return *this = vect(c, c);
}


inline vect< 2, __m128 >&
vect< 2, __m128 >::crossr(
	const base::vect< float, 2, __m128 >& src)
{
	return this->cross(*this, src);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// class vect<3>
// specialization of vect for DIMENSION = 3, NATIVE_T = __m128
////////////////////////////////////////////////////////////////////////////////////////////////////

template <>
class vect< 3, __m128 > : public vectr< vect< 3, __m128 >, 3, __m128 >
{
public:

	vect() {
	}

	explicit vect(
		const base::vect< float, 3, __m128 >& src)
	: vectr< vect, 3, __m128 >(src) {
	}

	explicit vect(
		const float (& src)[3],
		const bool zero_first = false);

	explicit vect(
		const float c0,
		const float c1,
		const float c2,
		const bool zero_first = false);

	// negative of argument
	vect& negate(
		const base::vect< float, 3, __m128 >& src);

	// negative of this
	using vectr< vect, 3, __m128 >::negate;

	// sum of arguments
	vect& add(
		const base::vect< float, 3, __m128 >& src0,
		const base::vect< float, 3, __m128 >& src1);

	using vectr< vect, 3, __m128 >::add;

	// difference between arguments
	vect& sub(
		const base::vect< float, 3, __m128 >& src0,
		const base::vect< float, 3, __m128 >& src1);

	using vectr< vect, 3, __m128 >::sub;

	// product of argument and scalar argument
	vect& mul(
		const base::vect< float, 3, __m128 >& src,
		const float c);

	// product of arguments
	vect& mul(
		const base::vect< float, 3, __m128 >& src0,
		const base::vect< float, 3, __m128 >& src1);

	// product of argument and row-major operator of R4
	vect& mul(
		const base::vect< float, 3, __m128 >& src,
		const base::matx< float, 4, __m128 >& op);

	using vectr< vect, 3, __m128 >::mul;

	// product of argument and row-major operator of R4; an extra element of argument assumed 1
	vect& mulH(
		const base::vect< float, 3, __m128 >& src,
		const base::matx< float, 4, __m128 >& op);

	using vectr< vect, 3, __m128 >::mulH;

	// division of first by second argument
	vect& div(
		const base::vect< float, 3, __m128 >& src0,
		const base::vect< float, 3, __m128 >& src1);

	using vectr< vect, 3, __m128 >::div;

	// reciprocal of argument
	vect& rcp(
		const base::vect< float, 3, __m128 >& src);

	using vectr< vect, 3, __m128 >::rcp;

	// cross-product of this and argument
	vect& crossr(
		const base::vect< float, 3, __m128 >& src);

	// cross-product of arguments
	vect& cross(
		const base::vect< float, 3, __m128 >& src0,
		const base::vect< float, 3, __m128 >& src1);

	// normalise argument
	vect& normalise(
		const base::vect< float, 3, __m128 >& src);

	using vectr< vect, 3, __m128 >::normalise;

	// dot product
	float dot(
		const base::vect< float, 3, __m128 >& src) const;
};


inline vect< 3, __m128 >::vect(
	const float (& src)[3],
	const bool zero_first)
: vectr< vect, 3, __m128 >(src, zero_first)
{
}


inline vect< 3, __m128 >::vect(
	const float c0,
	const float c1,
	const float c2,
	const bool zero_first)
{
	const bool zero_init = this->dimension != this->native_dimension && zero_first;

	if (zero_init)
		this->zero();

	this->set(0, c0);
	this->set(1, c1);
	this->set(2, c2);

	if (zero_init)
		return;

	for (size_t i = this->dimension; i < this->native_dimension; ++i)
		this->set_native(i, c2);
}


inline vect< 3, __m128 >&
vect< 3, __m128 >::negate(
	const base::vect< float, 3, __m128 >& src)
{
	this->setn(0, _mm_xor_ps(src.getn(), _mm_set1_ps(-0.f)));

	return *this;
}


inline vect< 3, __m128 >&
vect< 3, __m128 >::add(
	const base::vect< float, 3, __m128 >& src0,
	const base::vect< float, 3, __m128 >& src1)
{
	this->setn(0, _mm_add_ps(src0.getn(), src1.getn()));

	return *this;
}


inline vect< 3, __m128 >&
vect< 3, __m128 >::sub(
	const base::vect< float, 3, __m128 >& src0,
	const base::vect< float, 3, __m128 >& src1)
{
	this->setn(0, _mm_sub_ps(src0.getn(), src1.getn()));

	return *this;
}


inline vect< 3, __m128 >&
vect< 3, __m128 >::mul(
	const base::vect< float, 3, __m128 >& src,
	const float c)
{
	this->setn(0, _mm_mul_ps(_mm_set1_ps(c), src.getn()));

	return *this;
}


inline vect< 3, __m128 >&
vect< 3, __m128 >::mul(
	const base::vect< float, 3, __m128 >& src0,
	const base::vect< float, 3, __m128 >& src1)
{
	this->setn(0, _mm_mul_ps(src0.getn(), src1.getn()));

	return *this;
}


inline vect< 3, __m128 >&
vect< 3, __m128 >::mul(
	const base::vect< float, 3, __m128 >& src,
	const base::matx< float, 4, __m128 >& op)
{
	this->setn(0,            _mm_mul_ps(_mm_shuffle_ps(src.getn(), src.getn(), _MM_SHUFFLE(0, 0, 0, 0)), op[0].getn()));
	this->setn(0, _mm_add_ps(_mm_mul_ps(_mm_shuffle_ps(src.getn(), src.getn(), _MM_SHUFFLE(1, 1, 1, 1)), op[1].getn()), this->getn()));
	this->setn(0, _mm_add_ps(_mm_mul_ps(_mm_shuffle_ps(src.getn(), src.getn(), _MM_SHUFFLE(2, 2, 2, 2)), op[2].getn()), this->getn()));

	return *this;
}


inline vect< 3, __m128 >&
vect< 3, __m128 >::mulH(
	const base::vect< float, 3, __m128 >& src,
	const base::matx< float, 4, __m128 >& op)
{
	this->setn(0, op[3].getn());
	this->setn(0, _mm_add_ps(_mm_mul_ps(_mm_shuffle_ps(src.getn(), src.getn(), _MM_SHUFFLE(0, 0, 0, 0)), op[0].getn()), this->getn()));
	this->setn(0, _mm_add_ps(_mm_mul_ps(_mm_shuffle_ps(src.getn(), src.getn(), _MM_SHUFFLE(1, 1, 1, 1)), op[1].getn()), this->getn()));
	this->setn(0, _mm_add_ps(_mm_mul_ps(_mm_shuffle_ps(src.getn(), src.getn(), _MM_SHUFFLE(2, 2, 2, 2)), op[2].getn()), this->getn()));

	return *this;
}


inline vect< 3, __m128 >&
vect< 3, __m128 >::rcp(
	const base::vect< float, 3, __m128 >& src)
{
	// one-iteration Newton-Raphson refinement for 1 / x:
	// est' = est + est * (1 - est * x)
	const __m128 est = _mm_rcp_ps(src.getn()); // max relative error = 1.5 * 2^-12
	const __m128 one = { 1.f, 1.f, 1.f, 1.f };
	__m128 rcp;
	rcp = _mm_sub_ps(one, _mm_mul_ps(est, src.getn()));
	rcp = _mm_add_ps(est, _mm_mul_ps(est, rcp));

	this->setn(0, rcp);

	return *this;
}


inline vect< 3, __m128 >&
vect< 3, __m128 >::div(
	const base::vect< float, 3, __m128 >& src0,
	const base::vect< float, 3, __m128 >& src1)
{
#if VECT_SIMD_DIV_AS_RCP == 1
	// one-iteration Newton-Raphson refinement for 1 / x:
	// est' = est + est * (1 - est * x)
	const __m128 est = _mm_rcp_ps(src1.getn()); // max relative error = 1.5 * 2^-12
	const __m128 one = { 1.f, 1.f, 1.f, 1.f };
	__m128 rcp;
	rcp = _mm_sub_ps(one, _mm_mul_ps(est, src1.getn()));
	rcp = _mm_add_ps(est, _mm_mul_ps(est, rcp));

	this->setn(0, _mm_mul_ps(src0.getn(), rcp));

	// since we used a reciprocal to get Q = A / B, compute the residual R = A - B * Q to obtain the correctly rounded Q' = Q + R * rcp
	// note: in clang 3.4 compile-time fp constants seem to be computed using round-down/round-to-zero under __OPTIMIZE__, which is
	// inconsistent with the run-time default round-to-nearest and leads to unit-test failure under said compiler
	const __m128 res = _mm_sub_ps(src0.getn(), _mm_mul_ps(src1.getn(), this->getn()));
	this->setn(0, _mm_add_ps(this->getn(), _mm_mul_ps(res, rcp)));

#else // VECT_SIMD_DIV_AS_RCP == 1
	this->setn(0, _mm_div_ps(src0.getn(), src1.getn()));

#endif // VECT_SIMD_DIV_AS_RCP == 1
	return *this;
}


inline vect< 3, __m128 >&
vect< 3, __m128 >::cross(
	const base::vect< float, 3, __m128 >& src0,
	const base::vect< float, 3, __m128 >& src1)
{
	const __m128 s0 = _mm_shuffle_ps(src0.getn(), src0.getn(), _MM_SHUFFLE(3, 0, 2, 1));
	const __m128 s1 = _mm_shuffle_ps(src1.getn(), src1.getn(), _MM_SHUFFLE(3, 0, 2, 1));
	const __m128 p0 = _mm_mul_ps(src0.getn(), s1);
	const __m128 p1 = _mm_mul_ps(src1.getn(), s0);
	const __m128 r  = _mm_sub_ps(p0, p1);

	this->setn(0, _mm_shuffle_ps(r, r, _MM_SHUFFLE(3, 0, 2, 1)));

	return *this;
}


inline vect< 3, __m128 >&
vect< 3, __m128 >::crossr(
	const base::vect< float, 3, __m128 >& src)
{
	return *this = vect().cross(*this, src);
}


inline vect< 3, __m128 >&
vect< 3, __m128 >::normalise(
	const base::vect< float, 3, __m128 >& src)
{
#if __SSE4_1__ == 1
	const __m128 sqr = _mm_dp_ps(src.getn(), src.getn(), 0x7f);

#else
	const __m128 prod = _mm_mul_ps(src.getn(), src.getn());
	const __m128 sqr = _mm_add_ps(_mm_add_ps(_mm_shuffle_ps(prod, prod, 0), _mm_shuffle_ps(prod, prod, 0x55)), _mm_shuffle_ps(prod, prod, 0xaa));

#endif

#if VECT_SIMD_SQRT_DIV_AS_RSQRT == 1
	// one-iteration Newton-Raphson refinement for 1 / sqrt(x):
	// est' = -0.5 * est * ((x * est) * est - 3)
	const __m128 est = _mm_rsqrt_ps(sqr); // max relative error = 1.5 * 2^-12
	const __m128 rln = _mm_mul_ps(_mm_mul_ps(est, _mm_set1_ps(-.5f)), _mm_sub_ps(_mm_mul_ps(_mm_mul_ps(sqr, est), est), _mm_set1_ps(3.f)));

	this->setn(0, _mm_mul_ps(src.getn(), rln));

#else
	const __m128 len = _mm_sqrt_ps(sqr);

	this->setn(0, _mm_div_ps(src.getn(), len));

#endif
	return *this;
}


inline float
vect< 3, __m128 >::dot(
	const base::vect< float, 3, __m128 >& src) const
{
#if __SSE4_1__ == 1
	return _mm_cvtss_f32(_mm_dp_ps(this->getn(), src.getn(), 0x71));

#else
	return vectr< vect, 3, __m128 >::dot(src);

#endif
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// class vect<4>
// specialization of vect for DIMENSION = 4, NATIVE_T = __m128
////////////////////////////////////////////////////////////////////////////////////////////////////

template <>
class vect< 4, __m128 > : public vectr< vect< 4, __m128 >, 4, __m128 >
{
public:

	vect() {
	}

	explicit vect(
		const base::vect< float, 4, __m128 >& src)
	: vectr< vect, 4, __m128 >(src) {
	}

	explicit vect(
		const float (& src)[4],
		const bool zero_first = false);

	explicit vect(
		const float c0,
		const float c1,
		const float c2,
		const float c3,
		const bool zero_first = false);

	// negative of argument
	vect& negate(
		const base::vect< float, 4, __m128 >& src);

	// negative of this
	using vectr< vect, 4, __m128 >::negate;

	// sum of arguments
	vect& add(
		const base::vect< float, 4, __m128 >& src0,
		const base::vect< float, 4, __m128 >& src1);

	using vectr< vect, 4, __m128 >::add;

	// difference between arguments
	vect& sub(
		const base::vect< float, 4, __m128 >& src0,
		const base::vect< float, 4, __m128 >& src1);

	using vectr< vect, 4, __m128 >::sub;

	// product of argument and scalar argument
	vect& mul(
		const base::vect< float, 4, __m128 >& src,
		const float c);

	// product of arguments
	vect& mul(
		const base::vect< float, 4, __m128 >& src0,
		const base::vect< float, 4, __m128 >& src1);

	// product of argument and row-major operator
	vect& mul(
		const base::vect< float, 4, __m128 >& src,
		const base::matx< float, 4, __m128 >& op);

	using vectr< vect, 4, __m128 >::mul;

	// product of argument and row-major operator; last element of argument assumed 1
	vect& mulH(
		const base::vect< float, 4, __m128 >& src,
		const base::matx< float, 4, __m128 >& op);

	using vectr< vect, 4, __m128 >::mulH;

	// division of first by second argument
	vect& div(
		const base::vect< float, 4, __m128 >& src0,
		const base::vect< float, 4, __m128 >& src1);

	using vectr< vect, 4, __m128 >::div;

	// reciprocal of argument
	vect& rcp(
		const base::vect< float, 4, __m128 >& src);

	using vectr< vect, 4, __m128 >::rcp;

	// cross-product of this and argument, taken as 3-component
	vect& crossr(
		const base::vect< float, 4, __m128 >& src);

	// cross-product of arguments, taken as 3-component
	vect& cross(
		const base::vect< float, 4, __m128 >& src0,
		const base::vect< float, 4, __m128 >& src1);

	// normalise argument
	vect& normalise(
		const base::vect< float, 4, __m128 >& src);

	using vectr< vect, 4, __m128 >::normalise;

	// dot product
	float dot(
		const base::vect< float, 4, __m128 >& src) const;
};


inline vect< 4, __m128 >::vect(
	const float (& src)[4],
	const bool zero_first)
: vectr< vect, 4, __m128 >(src, zero_first)
{
}


inline vect< 4, __m128 >::vect(
	const float c0,
	const float c1,
	const float c2,
	const float c3,
	const bool)
{
	this->set(0, c0);
	this->set(1, c1);
	this->set(2, c2);
	this->set(3, c3);
}


inline vect< 4, __m128 >&
vect< 4, __m128 >::negate(
	const base::vect< float, 4, __m128 >& src)
{
	this->setn(0, _mm_xor_ps(src.getn(), _mm_set1_ps(-0.f)));

	return *this;
}


inline vect< 4, __m128 >&
vect< 4, __m128 >::add(
	const base::vect< float, 4, __m128 >& src0,
	const base::vect< float, 4, __m128 >& src1)
{
	this->setn(0, _mm_add_ps(src0.getn(), src1.getn()));

	return *this;
}


inline vect< 4, __m128 >&
vect< 4, __m128 >::sub(
	const base::vect< float, 4, __m128 >& src0,
	const base::vect< float, 4, __m128 >& src1)
{
	this->setn(0, _mm_sub_ps(src0.getn(), src1.getn()));

	return *this;
}


inline vect< 4, __m128 >&
vect< 4, __m128 >::mul(
	const base::vect< float, 4, __m128 >& src,
	const float c)
{
	this->setn(0, _mm_mul_ps(_mm_set1_ps(c), src.getn()));

	return *this;
}


inline vect< 4, __m128 >&
vect< 4, __m128 >::mul(
	const base::vect< float, 4, __m128 >& src0,
	const base::vect< float, 4, __m128 >& src1)
{
	this->setn(0, _mm_mul_ps(src0.getn(), src1.getn()));

	return *this;
}


inline vect< 4, __m128 >&
vect< 4, __m128 >::mul(
	const base::vect< float, 4, __m128 >& src,
	const base::matx< float, 4, __m128 >& op)
{
	this->setn(0,            _mm_mul_ps(_mm_shuffle_ps(src.getn(), src.getn(), _MM_SHUFFLE(0, 0, 0, 0)), op[0].getn()));
	this->setn(0, _mm_add_ps(_mm_mul_ps(_mm_shuffle_ps(src.getn(), src.getn(), _MM_SHUFFLE(1, 1, 1, 1)), op[1].getn()), this->getn()));
	this->setn(0, _mm_add_ps(_mm_mul_ps(_mm_shuffle_ps(src.getn(), src.getn(), _MM_SHUFFLE(2, 2, 2, 2)), op[2].getn()), this->getn()));
	this->setn(0, _mm_add_ps(_mm_mul_ps(_mm_shuffle_ps(src.getn(), src.getn(), _MM_SHUFFLE(3, 3, 3, 3)), op[3].getn()), this->getn()));

	return *this;
}


inline vect< 4, __m128 >&
vect< 4, __m128 >::mulH(
	const base::vect< float, 4, __m128 >& src,
	const base::matx< float, 4, __m128 >& op)
{
	this->setn(0, op[3].getn());
	this->setn(0, _mm_add_ps(_mm_mul_ps(_mm_shuffle_ps(src.getn(), src.getn(), _MM_SHUFFLE(0, 0, 0, 0)), op[0].getn()), this->getn()));
	this->setn(0, _mm_add_ps(_mm_mul_ps(_mm_shuffle_ps(src.getn(), src.getn(), _MM_SHUFFLE(1, 1, 1, 1)), op[1].getn()), this->getn()));
	this->setn(0, _mm_add_ps(_mm_mul_ps(_mm_shuffle_ps(src.getn(), src.getn(), _MM_SHUFFLE(2, 2, 2, 2)), op[2].getn()), this->getn()));

	return *this;
}


inline vect< 4, __m128 >&
vect< 4, __m128 >::rcp(
	const base::vect< float, 4, __m128 >& src)
{
	// one-iteration Newton-Raphson refinement for 1 / x:
	// est' = est + est * (1 - est * x)
	const __m128 est = _mm_rcp_ps(src.getn()); // max relative error = 1.5 * 2^-12
	const __m128 one = { 1.f, 1.f, 1.f, 1.f };
	__m128 rcp;
	rcp = _mm_sub_ps(one, _mm_mul_ps(est, src.getn()));
	rcp = _mm_add_ps(est, _mm_mul_ps(est, rcp));

	this->setn(0, rcp);

	return *this;
}


inline vect< 4, __m128 >&
vect< 4, __m128 >::div(
	const base::vect< float, 4, __m128 >& src0,
	const base::vect< float, 4, __m128 >& src1)
{
#if VECT_SIMD_DIV_AS_RCP == 1
	// one-iteration Newton-Raphson refinement for 1 / x:
	// est' = est + est * (1 - est * x)
	const __m128 est = _mm_rcp_ps(src1.getn()); // max relative error = 1.5 * 2^-12
	const __m128 one = { 1.f, 1.f, 1.f, 1.f };
	__m128 rcp;
	rcp = _mm_sub_ps(one, _mm_mul_ps(est, src1.getn()));
	rcp = _mm_add_ps(est, _mm_mul_ps(est, rcp));

	this->setn(0, _mm_mul_ps(src0.getn(), rcp));

	// since we used a reciprocal to get Q = A / B, compute the residual R = A - B * Q to obtain the correctly rounded Q' = Q + R * rcp
	// note: in clang 3.4 compile-time fp constants seem to be computed using round-down/round-to-zero under __OPTIMIZE__, which is
	// inconsistent with the run-time default round-to-nearest and leads to unit-test failure under said compiler
	const __m128 res = _mm_sub_ps(src0.getn(), _mm_mul_ps(src1.getn(), this->getn()));
	this->setn(0, _mm_add_ps(this->getn(), _mm_mul_ps(res, rcp)));

#else // VECT_SIMD_DIV_AS_RCP == 1
	this->setn(0, _mm_div_ps(src0.getn(), src1.getn()));

#endif // VECT_SIMD_DIV_AS_RCP == 1
	return *this;
}


inline vect< 4, __m128 >&
vect< 4, __m128 >::cross(
	const base::vect< float, 4, __m128 >& src0,
	const base::vect< float, 4, __m128 >& src1)
{
	const __m128 s0 = _mm_shuffle_ps(src0.getn(), src0.getn(), _MM_SHUFFLE(3, 0, 2, 1));
	const __m128 s1 = _mm_shuffle_ps(src1.getn(), src1.getn(), _MM_SHUFFLE(3, 0, 2, 1));
	const __m128 p0 = _mm_mul_ps(src0.getn(), s1);
	const __m128 p1 = _mm_mul_ps(src1.getn(), s0);
	const __m128 r  = _mm_sub_ps(p0, p1);

	this->setn(0, _mm_shuffle_ps(r, r, _MM_SHUFFLE(3, 0, 2, 1)));

	return *this;
}


inline vect< 4, __m128 >&
vect< 4, __m128 >::crossr(
	const base::vect< float, 4, __m128 >& src)
{
	return *this = vect().cross(*this, src);
}


inline vect< 4, __m128 >&
vect< 4, __m128 >::normalise(
	const base::vect< float, 4, __m128 >& src)
{
#if __SSE4_1__ == 1
	const __m128 sqr = _mm_dp_ps(src.getn(), src.getn(), 0xff);

#else
	const __m128 prod = _mm_mul_ps(src.getn(), src.getn());
	const __m128 sqr = _mm_add_ps(_mm_add_ps(_mm_add_ps(_mm_shuffle_ps(prod, prod, 0), _mm_shuffle_ps(prod, prod, 0x55)), _mm_shuffle_ps(prod, prod, 0xaa)), _mm_shuffle_ps(prod, prod, 0xff));

#endif

#if VECT_SIMD_SQRT_DIV_AS_RSQRT == 1
	// one-iteration Newton-Raphson refinement for 1 / sqrt(x):
	// est' = -0.5 * est * ((x * est) * est - 3)
	const __m128 est = _mm_rsqrt_ps(sqr); // max relative error = 1.5 * 2^-12
	const __m128 rln = _mm_mul_ps(_mm_mul_ps(est, _mm_set1_ps(-.5f)), _mm_sub_ps(_mm_mul_ps(_mm_mul_ps(sqr, est), est), _mm_set1_ps(3.f)));

	this->setn(0, _mm_mul_ps(src.getn(), rln));

#else
	const __m128 len = _mm_sqrt_ps(sqr);

	this->setn(0, _mm_div_ps(src.getn(), len));

#endif
	return *this;
}


inline float
vect< 4, __m128 >::dot(
	const base::vect< float, 4, __m128 >& src) const
{
#if __SSE4_1__ == 1
	return _mm_cvtss_f32(_mm_dp_ps(this->getn(), src.getn(), 0xf1));

#else
	return vectr< vect, 4, __m128 >::dot(src);

#endif
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// class hamilton
// quaternion abstraction, specialized for rotations, i.e. unit quaternion. expressed from an angle
// and an axis of rotation, q = cos(a/2) + i (x * sin(a/2)) + j (y * sin(a/2)) + k (z * sin(a/2))
// herein quaternions use an x, y, z, w vector layout, where the last component is the scalar part
////////////////////////////////////////////////////////////////////////////////////////////////////

template < typename NATIVE_T >
class hamilton : public vectr< hamilton< NATIVE_T >, 4, NATIVE_T >
{
public:

	hamilton() {
	}

	explicit hamilton(
		const base::vect< float, 4, NATIVE_T >& src)
	: vectr< hamilton, 4, NATIVE_T >(src) {
	}

	explicit hamilton(
		const float (& src)[4],
		const bool zero_first = false);

	explicit hamilton(
		const float a,
		const base::vect< float, 3, NATIVE_T >& axis);

	explicit hamilton(
		const float x,
		const float y,
		const float z,
		const float w);

	// quaternion product of arguments
	hamilton& qmul(
		const hamilton& src0,
		const hamilton& src1);

	// quaternion product of this and argument
	hamilton& qmulr(
		const hamilton& src);

	// quaternion product of argument and this
	hamilton& qmull(
		const hamilton& src);

	// conjugate of this
	hamilton& conj();

	// conjugate of argument
	hamilton& conj(
		const hamilton& src);
};


template < typename NATIVE_T >
inline hamilton< NATIVE_T >::hamilton(
	const float (& src)[4],
	const bool zero_first)
: vectr< hamilton, 4, NATIVE_T >(src, zero_first)
{
}


template < typename NATIVE_T >
inline hamilton< NATIVE_T >::hamilton(
	const float a,
	const base::vect< float, 3, NATIVE_T >& axis)
{
	const float sin_ha = std::sin(a * .5f);
	const float cos_ha = std::cos(a * .5f);
	const vect< 3, NATIVE_T > ijk = vect< 3, NATIVE_T >().mul(axis, sin_ha);

	this->set(0, ijk[0]);
	this->set(1, ijk[1]);
	this->set(2, ijk[2]);
	this->set(3, cos_ha);
}


template < typename NATIVE_T >
inline hamilton< NATIVE_T >::hamilton(
	const float x,
	const float y,
	const float z,
	const float w)
{
	this->set(0, x);
	this->set(1, y);
	this->set(2, z);
	this->set(3, w);
}


template < typename NATIVE_T >
inline hamilton< NATIVE_T >&
hamilton< NATIVE_T >::qmul(
	const hamilton< NATIVE_T >& src0,
	const hamilton< NATIVE_T >& src1)
{
	return *this = hamilton(
		src0[3] * src1[0] + src0[0] * src1[3] + src0[1] * src1[2] - src0[2] * src1[1],
		src0[3] * src1[1] - src0[0] * src1[2] + src0[1] * src1[3] + src0[2] * src1[0],
		src0[3] * src1[2] + src0[0] * src1[1] - src0[1] * src1[0] + src0[2] * src1[3],
		src0[3] * src1[3] - src0[0] * src1[0] - src0[1] * src1[1] - src0[2] * src1[2]);
}


template < typename NATIVE_T >
inline hamilton< NATIVE_T >&
hamilton< NATIVE_T >::qmulr(
	const hamilton< NATIVE_T >& src)
{
	return this->qmul(*this, src);
}


template < typename NATIVE_T >
inline hamilton< NATIVE_T >&
hamilton< NATIVE_T >::qmull(
	const hamilton< NATIVE_T >& src)
{
	return this->qmul(src, *this);
}


template < typename NATIVE_T >
inline hamilton< NATIVE_T >&
hamilton< NATIVE_T>::conj(
	const hamilton< NATIVE_T >& src)
{
	return *this = hamilton().mul(src, vect< 4, NATIVE_T >(-1.f, -1.f, -1.f,  1.f));
}


template < typename NATIVE_T >
inline hamilton< NATIVE_T >&
hamilton< NATIVE_T>::conj()
{
	return this->conj(*this);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// class protomatx
// dimensionality-agnostic square row-major matrix of floats; subclass as:
//
//		class myMatx : public protomatx< myDimension, myNative, myMatx >
//
////////////////////////////////////////////////////////////////////////////////////////////////////

template < typename SUBCLASS_T, size_t DIMENSION, typename NATIVE_T >
class protomatx : public base::matx< float, DIMENSION, NATIVE_T >
{
public:

	typedef base::matx< float, DIMENSION, NATIVE_T > basetype;
	typedef SUBCLASS_T subclass;

	protomatx() {
	}

	explicit protomatx(
		const base::matx< float, DIMENSION, NATIVE_T >& src)
	: base::matx< float, DIMENSION, NATIVE_T>(src) {
	}

	explicit protomatx(
		const float (& src)[DIMENSION][DIMENSION]);

	operator SUBCLASS_T&();

	operator const SUBCLASS_T&() const;

	// identity mutator
	SUBCLASS_T& identity();

	// transpose of argument
	SUBCLASS_T& transpose(
		const base::matx< float, DIMENSION, NATIVE_T >& src);

	// transpose of this
	SUBCLASS_T& transpose();

	// product of arguments
	SUBCLASS_T& mul(
		const base::matx< float, DIMENSION, NATIVE_T >& src0,
		const base::matx< float, DIMENSION, NATIVE_T >& src1);

	// product of this and argument
	SUBCLASS_T& mulr(
		const base::matx< float, DIMENSION, NATIVE_T >& src);

	// product of argument and this
	SUBCLASS_T& mull(
		const base::matx< float, DIMENSION, NATIVE_T >& src);

	// product of argument and scalar argument
	SUBCLASS_T& mul(
		const base::matx< float, DIMENSION, NATIVE_T >& src,
		const float c);

	// product of this and scalar argument
	SUBCLASS_T& mul(
		const float c);
};


template < typename SUBCLASS_T, size_t DIMENSION, typename NATIVE_T >
inline
protomatx< SUBCLASS_T, DIMENSION, NATIVE_T >::operator SUBCLASS_T&()
{
	return *static_cast< SUBCLASS_T* >(this);
}


template < typename SUBCLASS_T, size_t DIMENSION, typename NATIVE_T >
inline
protomatx< SUBCLASS_T, DIMENSION, NATIVE_T >::operator const SUBCLASS_T&() const
{
	return *static_cast< const SUBCLASS_T* >(this);
}


template < typename SUBCLASS_T, size_t DIMENSION, typename NATIVE_T >
protomatx< SUBCLASS_T, DIMENSION, NATIVE_T >::protomatx(
	const float (& src)[DIMENSION][DIMENSION])
{
	SUBCLASS_T* const subthis = static_cast< SUBCLASS_T* >(this);

	for (size_t i = 0; i < DIMENSION; ++i)
		for (size_t j = 0; j < DIMENSION; ++j)
			subthis->set(i, j, src[i][j]);
}


template < typename SUBCLASS_T, size_t DIMENSION, typename NATIVE_T >
inline SUBCLASS_T&
protomatx< SUBCLASS_T, DIMENSION, NATIVE_T >::identity()
{
	for (size_t i = 0; i < DIMENSION; ++i)
	{
		for (size_t j = 0; j < i; ++j)
			this->set(i, j, 0.f);

		this->set(i, i, 1.f);

		for (size_t j = i + 1; j < DIMENSION; ++j)
			this->set(i, j, 0.f);
	}

	return *this;
}


template < typename SUBCLASS_T, size_t DIMENSION, typename NATIVE_T >
inline SUBCLASS_T&
protomatx< SUBCLASS_T, DIMENSION, NATIVE_T >::transpose(
	const base::matx< float, DIMENSION, NATIVE_T >& src)
{
	SUBCLASS_T* const subthis = static_cast< SUBCLASS_T* >(this);

	for (size_t i = 0; i < DIMENSION; ++i)
		for (size_t j = 0; j < DIMENSION; ++j)
			subthis->set(j, i, src[i][j]);

	return *this;
}


template < typename SUBCLASS_T, size_t DIMENSION, typename NATIVE_T >
inline SUBCLASS_T&
protomatx< SUBCLASS_T, DIMENSION, NATIVE_T >::transpose()
{
	return *this = SUBCLASS_T().transpose(*this);
}


template < typename SUBCLASS_T, size_t DIMENSION, typename NATIVE_T >
inline SUBCLASS_T&
protomatx< SUBCLASS_T, DIMENSION, NATIVE_T >::mul(
	const base::matx< float, DIMENSION, NATIVE_T >& src0,
	const base::matx< float, DIMENSION, NATIVE_T >& src1)
{
	SUBCLASS_T* const subthis = static_cast< SUBCLASS_T* >(this);

	for (size_t i = 0; i < DIMENSION; ++i)
		subthis->set(i, vect< DIMENSION, NATIVE_T >().mul(src0[i], src1));

	return *this;
}


template < typename SUBCLASS_T, size_t DIMENSION, typename NATIVE_T >
inline SUBCLASS_T&
protomatx< SUBCLASS_T, DIMENSION, NATIVE_T >::mulr(
	const base::matx< float, DIMENSION, NATIVE_T >& src)
{
	return *this = SUBCLASS_T().mul(*this, src);
}


template < typename SUBCLASS_T, size_t DIMENSION, typename NATIVE_T >
inline SUBCLASS_T&
protomatx< SUBCLASS_T, DIMENSION, NATIVE_T >::mull(
	const base::matx< float, DIMENSION, NATIVE_T >& src)
{
	return *this = SUBCLASS_T().mul(src, *this);
}


template < typename SUBCLASS_T, size_t DIMENSION, typename NATIVE_T >
inline SUBCLASS_T&
protomatx< SUBCLASS_T, DIMENSION, NATIVE_T >::mul(
	const base::matx< float, DIMENSION, NATIVE_T >& src,
	const float c)
{
	SUBCLASS_T* const subthis = static_cast< SUBCLASS_T* >(this);

	for (size_t i = 0; i < DIMENSION; ++i)
		subthis->set(i, vect< DIMENSION, NATIVE_T >().mul(src[i], c));

	return *this;
}


template < typename SUBCLASS_T, size_t DIMENSION, typename NATIVE_T >
inline SUBCLASS_T&
protomatx< SUBCLASS_T, DIMENSION, NATIVE_T >::mul(
	const float c)
{
	return *this = SUBCLASS_T().mul(*this, c);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// class matx
// instantiatable, dimensionality-agnostic square matrix of floats
////////////////////////////////////////////////////////////////////////////////////////////////////

template < size_t DIMENSION, typename NATIVE_T >
class matx : public protomatx< matx< DIMENSION, NATIVE_T >, DIMENSION, NATIVE_T >
{
public:

	matx() {
	}

	explicit matx(
		const base::matx< float, DIMENSION, NATIVE_T >& src)
	: protomatx< matx, DIMENSION, NATIVE_T>(src) {
	}

	explicit matx(
		const float (& src)[DIMENSION][DIMENSION]);
};


template < size_t DIMENSION, typename NATIVE_T >
inline matx< DIMENSION, NATIVE_T >::matx(
	const float (& src)[DIMENSION][DIMENSION])
: protomatx< matx, DIMENSION, NATIVE_T >(src)
{
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// class matx<3>
// specialization of matx for DIMENSION = 3, NATIVE_T = __m128
////////////////////////////////////////////////////////////////////////////////////////////////////

template <>
class matx< 3, __m128 > : public protomatx< matx< 3, __m128 >, 3, __m128 >
{
public:

	matx() {
	}

	explicit matx(
		const base::matx< float, 3, __m128 >& src)
	: protomatx< matx, 3, __m128 >(src) {
	}

	explicit matx(
		const float (& src)[3][3]);

	explicit matx(
		const float c00, const float c01, const float c02,
		const float c10, const float c11, const float c12,
		const float c20, const float c21, const float c22);

	explicit matx(
		const hamilton< __m128 >& q);

	// identity
	matx& identity();
};


inline matx< 3, __m128 >::matx(
	const float (& src)[3][3])
: protomatx< matx, 3, __m128 >(src)
{
}


inline matx< 3, __m128 >::matx(
	const float c00, const float c01, const float c02,
	const float c10, const float c11, const float c12,
	const float c20, const float c21, const float c22)
{
	this->set(0, 0, c00);
	this->set(0, 1, c01);
	this->set(0, 2, c02);

	this->set(1, 0, c10);
	this->set(1, 1, c11);
	this->set(1, 2, c12);

	this->set(2, 0, c20);
	this->set(2, 1, c21);
	this->set(2, 2, c22);
}


inline matx< 3, __m128 >::matx(
	const hamilton< __m128 >& q)
{
	this->set(0, 0, 1.f - 2.f * (q[1] * q[1] + q[2] * q[2]));
	this->set(0, 1,       2.f * (q[0] * q[1] + q[2] * q[3]));
	this->set(0, 2,       2.f * (q[0] * q[2] - q[1] * q[3]));

	this->set(1, 0,       2.f * (q[0] * q[1] - q[2] * q[3]));
	this->set(1, 1, 1.f - 2.f * (q[0] * q[0] + q[2] * q[2]));
	this->set(1, 2,       2.f * (q[1] * q[2] + q[0] * q[3]));

	this->set(2, 0,       2.f * (q[0] * q[2] + q[1] * q[3]));
	this->set(2, 1,       2.f * (q[1] * q[2] - q[0] * q[3]));
	this->set(2, 2, 1.f - 2.f * (q[0] * q[0] + q[1] * q[1]));
}


inline matx< 3, __m128 >&
matx< 3, __m128 >::identity()
{
#if __SSE4_1__ == 1
	const __m128 one = _mm_set_ss(1.f);
	this->setn(0, 0, _mm_insert_ps(this->getn(0, 0), one, 0x0e));
	this->setn(1, 0, _mm_insert_ps(this->getn(1, 0), one, 0x1d));
	this->setn(2, 0, _mm_insert_ps(this->getn(2, 0), one, 0x2b));

#else
	this->setn(0, 0, (__m128) { 1.f, 0.f, 0.f, 0.f });
	this->setn(1, 0, (__m128) { 0.f, 1.f, 0.f, 0.f });
	this->setn(2, 0, (__m128) { 0.f, 0.f, 1.f, 0.f });

#endif
	return *this;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// class matx<4>
// specialization of matx for DIMENSION = 4, NATIVE_T = __m128
////////////////////////////////////////////////////////////////////////////////////////////////////

template <>
class matx< 4, __m128 > : public protomatx< matx< 4, __m128 >, 4, __m128 >
{
public:

	matx() {
	}

	explicit matx(
		const base::matx< float, 4, __m128 >& src)
	: protomatx< matx, 4, __m128 >(src) {
	}

	explicit matx(
		const float (& src)[4][4]);

	explicit matx(
		const float c00, const float c01, const float c02, const float c03,
		const float c10, const float c11, const float c12, const float c13,
		const float c20, const float c21, const float c22, const float c23,
		const float c30, const float c31, const float c32, const float c33);

	explicit matx(
		const hamilton< __m128 >& q);

	// identity
	matx& identity();

	// transpose of argument
	matx& transpose(
		const base::matx< float, 4, __m128 >& src);

	using protomatx< matx, 4, __m128 >::transpose;

	// inverse of this
	matx& inverse();

	// inverse of argument
	matx& inverse(
		const base::matx< float, 4, __m128 >& src);
};


inline matx< 4, __m128 >::matx(
	const float (& src)[4][4])
: protomatx< matx, 4, __m128 >(src)
{
}


inline matx< 4, __m128 >::matx(
	const float c00, const float c01, const float c02, const float c03,
	const float c10, const float c11, const float c12, const float c13,
	const float c20, const float c21, const float c22, const float c23,
	const float c30, const float c31, const float c32, const float c33)
{
	this->set(0, 0, c00);
	this->set(0, 1, c01);
	this->set(0, 2, c02);
	this->set(0, 3, c03);

	this->set(1, 0, c10);
	this->set(1, 1, c11);
	this->set(1, 2, c12);
	this->set(1, 3, c13);

	this->set(2, 0, c20);
	this->set(2, 1, c21);
	this->set(2, 2, c22);
	this->set(2, 3, c23);

	this->set(3, 0, c30);
	this->set(3, 1, c31);
	this->set(3, 2, c32);
	this->set(3, 3, c33);
}


inline matx< 4, __m128 >::matx(
	const hamilton< __m128 >& q)
{
	this->set(0, 0, 1.f - 2.f * (q[1] * q[1] + q[2] * q[2]));
	this->set(0, 1,       2.f * (q[0] * q[1] + q[2] * q[3]));
	this->set(0, 2,       2.f * (q[0] * q[2] - q[1] * q[3]));
	this->set(0, 3, 0.f);

	this->set(1, 0,       2.f * (q[0] * q[1] - q[2] * q[3]));
	this->set(1, 1, 1.f - 2.f * (q[0] * q[0] + q[2] * q[2]));
	this->set(1, 2,       2.f * (q[1] * q[2] + q[0] * q[3]));
	this->set(1, 3, 0.f);

	this->set(2, 0,       2.f * (q[0] * q[2] + q[1] * q[3]));
	this->set(2, 1,       2.f * (q[1] * q[2] - q[0] * q[3]));
	this->set(2, 2, 1.f - 2.f * (q[0] * q[0] + q[1] * q[1]));
	this->set(2, 3, 0.f);

	this->set(3, 0, 0.f);
	this->set(3, 1, 0.f);
	this->set(3, 2, 0.f);
	this->set(3, 3, 1.f);
}


inline matx< 4, __m128 >&
matx< 4, __m128 >::identity()
{
#if __SSE4_1__ == 1
	const __m128 one = _mm_set_ss(1.f);
	this->setn(0, 0, _mm_insert_ps(this->getn(0, 0), one, 0x0e));
	this->setn(1, 0, _mm_insert_ps(this->getn(1, 0), one, 0x1d));
	this->setn(2, 0, _mm_insert_ps(this->getn(2, 0), one, 0x2b));
	this->setn(3, 0, _mm_insert_ps(this->getn(3, 0), one, 0x37));

#else
	this->setn(0, 0, (__m128) { 1.f, 0.f, 0.f, 0.f });
	this->setn(1, 0, (__m128) { 0.f, 1.f, 0.f, 0.f });
	this->setn(2, 0, (__m128) { 0.f, 0.f, 1.f, 0.f });
	this->setn(3, 0, (__m128) { 0.f, 0.f, 0.f, 1.f });

#endif
	return *this;
}


inline matx< 4, __m128 >&
matx< 4, __m128 >::transpose(
	const base::matx< float, 4, __m128 >& src)
{
	const __m128 t0 = _mm_unpacklo_ps(src[0].getn(), src[1].getn());
	const __m128 t1 = _mm_unpacklo_ps(src[2].getn(), src[3].getn());
	const __m128 t2 = _mm_unpackhi_ps(src[0].getn(), src[1].getn());
	const __m128 t3 = _mm_unpackhi_ps(src[2].getn(), src[3].getn());
	this->setn(0, 0, _mm_movelh_ps(t0, t1));
	this->setn(1, 0, _mm_movehl_ps(t1, t0));
	this->setn(2, 0, _mm_movelh_ps(t2, t3));
	this->setn(3, 0, _mm_movehl_ps(t3, t2));

	return *this;
}


inline matx< 4, __m128 >&
matx< 4, __m128 >::inverse(
	const base::matx< float, 4, __m128 >& src)
{
	// compute cofactors of the transpose, Intel style (AP-928)
	__m128 row0, row1, row2, row3, tmp1;

	tmp1 = _mm_movelh_ps(src[0].getn(), src[1].getn());
	row1 = _mm_movelh_ps(src[2].getn(), src[3].getn());
	row0 = _mm_shuffle_ps(tmp1, row1, 0x88);
	row1 = _mm_shuffle_ps(row1, tmp1, 0xdd);

	tmp1 = _mm_movehl_ps(src[1].getn(), src[0].getn());
	row3 = _mm_movehl_ps(src[3].getn(), src[2].getn());
	row2 = _mm_shuffle_ps(tmp1, row3, 0x88);
	row3 = _mm_shuffle_ps(row3, tmp1, 0xdd);

	__m128 minor0, minor1, minor2, minor3;

	tmp1 = _mm_mul_ps(row2, row3);
	tmp1 = _mm_shuffle_ps(tmp1, tmp1, 0xb1);
	minor0 = _mm_mul_ps(row1, tmp1);
	minor1 = _mm_mul_ps(row0, tmp1);
	tmp1 = _mm_shuffle_ps(tmp1, tmp1, 0x4e);
	minor0 = _mm_sub_ps(_mm_mul_ps(row1, tmp1), minor0);
	minor1 = _mm_sub_ps(_mm_mul_ps(row0, tmp1), minor1);
	minor1 = _mm_shuffle_ps(minor1, minor1, 0x4e);

	tmp1 = _mm_mul_ps(row1, row2);
	tmp1 = _mm_shuffle_ps(tmp1, tmp1, 0xb1);
	minor0 = _mm_add_ps(_mm_mul_ps(row3, tmp1), minor0);
	minor3 = _mm_mul_ps(row0, tmp1);
	tmp1 = _mm_shuffle_ps(tmp1, tmp1, 0x4e);
	minor0 = _mm_sub_ps(minor0, _mm_mul_ps(row3, tmp1));
	minor3 = _mm_sub_ps(_mm_mul_ps(row0, tmp1), minor3);
	minor3 = _mm_shuffle_ps(minor3, minor3, 0x4e);

	tmp1 = _mm_mul_ps(_mm_shuffle_ps(row1, row1, 0x4e), row3);
	tmp1 = _mm_shuffle_ps(tmp1, tmp1, 0xb1);
	row2 = _mm_shuffle_ps(row2, row2, 0x4e);
	minor0 = _mm_add_ps(_mm_mul_ps(row2, tmp1), minor0);
	minor2 = _mm_mul_ps(row0, tmp1);
	tmp1 = _mm_shuffle_ps(tmp1, tmp1, 0x4e);
	minor0 = _mm_sub_ps(minor0, _mm_mul_ps(row2, tmp1));
	minor2 = _mm_sub_ps(_mm_mul_ps(row0, tmp1), minor2);
	minor2 = _mm_shuffle_ps(minor2, minor2, 0x4e);

	tmp1 = _mm_mul_ps(row0, row1);
	tmp1 = _mm_shuffle_ps(tmp1, tmp1, 0xb1);
	minor2 = _mm_add_ps(_mm_mul_ps(row3, tmp1), minor2);
	minor3 = _mm_sub_ps(_mm_mul_ps(row2, tmp1), minor3);
	tmp1 = _mm_shuffle_ps(tmp1, tmp1, 0x4e);
	minor2 = _mm_sub_ps(_mm_mul_ps(row3, tmp1), minor2);
	minor3 = _mm_sub_ps(minor3, _mm_mul_ps(row2, tmp1));

	tmp1 = _mm_mul_ps(row0, row3);
	tmp1 = _mm_shuffle_ps(tmp1, tmp1, 0xb1);
	minor1 = _mm_sub_ps(minor1, _mm_mul_ps(row2, tmp1));
	minor2 = _mm_add_ps(_mm_mul_ps(row1, tmp1), minor2);
	tmp1 = _mm_shuffle_ps(tmp1, tmp1, 0x4e);
	minor1 = _mm_add_ps(_mm_mul_ps(row2, tmp1), minor1);
	minor2 = _mm_sub_ps(minor2, _mm_mul_ps(row1, tmp1));

	tmp1 = _mm_mul_ps(row0, row2);
	tmp1 = _mm_shuffle_ps(tmp1, tmp1, 0xb1);
	minor1 = _mm_add_ps(_mm_mul_ps(row3, tmp1), minor1);
	minor3 = _mm_sub_ps(minor3, _mm_mul_ps(row1, tmp1));
	tmp1 = _mm_shuffle_ps(tmp1, tmp1, 0x4e);
	minor1 = _mm_sub_ps(minor1, _mm_mul_ps(row3, tmp1));
	minor3 = _mm_add_ps(_mm_mul_ps(row1, tmp1), minor3);

	__m128 det;
	det = _mm_mul_ps(row0, minor0);
	det = _mm_add_ps(_mm_shuffle_ps(det, det, 0x4e), det);
	det = _mm_add_ss(_mm_shuffle_ps(det, det, 0xb1), det);
	det = _mm_div_ss(_mm_set_ss(1.f), det);
	det = _mm_shuffle_ps(det, det, _MM_SHUFFLE(0, 0, 0, 0));

	this->setn(0, 0, _mm_mul_ps(det, minor0));
	this->setn(1, 0, _mm_mul_ps(det, minor1));
	this->setn(2, 0, _mm_mul_ps(det, minor2));
	this->setn(3, 0, _mm_mul_ps(det, minor3));

	return *this;
}


inline matx< 4, __m128 >&
matx< 4, __m128 >::inverse()
{
	return this->inverse(*this);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// final specializations (finalizations)
////////////////////////////////////////////////////////////////////////////////////////////////////

typedef ivect< 2, __m128i > ivect2;
typedef ivect< 3, __m128i > ivect3;
typedef ivect< 4, __m128i > ivect4;

typedef vect< 2, __m128 > vect2;
typedef vect< 3, __m128 > vect3;
typedef vect< 4, __m128 > vect4;

typedef hamilton< __m128 > quat;

typedef matx< 3, __m128 > matx3;
typedef matx< 4, __m128 > matx4;

} // namespace simd

#endif // vect_simd_sse_H__
