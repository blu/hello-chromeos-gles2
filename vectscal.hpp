#ifndef scal_vect_H__
#define scal_vect_H__

#include <stdint.h>
#include <cmath>
#include <cassert>
#include "vectbase.hpp"

namespace scal
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

	protovect()
	{
	}

	explicit protovect(
		const SCALAR_T (& src)[DIMENSION]);

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
	const SCALAR_T (& src)[DIMENSION])
{
	for (size_t i = 0; i < DIMENSION; ++i)
		this->set(i, src[i]);
}


template < typename SUBCLASS_T, typename SCALAR_T, size_t DIMENSION, typename NATIVE_T >
inline SUBCLASS_T&
protovect< SUBCLASS_T, SCALAR_T, DIMENSION, NATIVE_T >::mul(
	const base::vect< SCALAR_T, DIMENSION, NATIVE_T >& src,
	const SCALAR_T c)
{
	for (size_t i = 0; i < DIMENSION; ++i)
		this->set(i, src[i] * c);

	return *this;
}


template < typename SUBCLASS_T, typename SCALAR_T, size_t DIMENSION, typename NATIVE_T >
inline SUBCLASS_T&
protovect< SUBCLASS_T, SCALAR_T, DIMENSION, NATIVE_T >::mul(
	const SCALAR_T c)
{
	return *this = protovect().mul(*this, c);
}


template < typename SUBCLASS_T, typename SCALAR_T, size_t DIMENSION, typename NATIVE_T >
inline SUBCLASS_T&
protovect< SUBCLASS_T, SCALAR_T, DIMENSION, NATIVE_T >::negate(
	const base::vect< SCALAR_T, DIMENSION, NATIVE_T >& src)
{
	for (size_t i = 0; i < DIMENSION; ++i)
		this->set(i, -src[i]);

	return *this;
}


template < typename SUBCLASS_T, typename SCALAR_T, size_t DIMENSION, typename NATIVE_T >
inline SUBCLASS_T&
protovect< SUBCLASS_T, SCALAR_T, DIMENSION, NATIVE_T >::negate()
{
	return *this = protovect().negate(*this);
}


template < typename SUBCLASS_T, typename SCALAR_T, size_t DIMENSION, typename NATIVE_T >
inline SUBCLASS_T&
protovect< SUBCLASS_T, SCALAR_T, DIMENSION, NATIVE_T >::add(
	const base::vect< SCALAR_T, DIMENSION, NATIVE_T >& src0,
	const base::vect< SCALAR_T, DIMENSION, NATIVE_T >& src1)
{
	for (size_t i = 0; i < DIMENSION; ++i)
		this->set(i, src0[i] + src1[i]);

	return *this;
}


template < typename SUBCLASS_T, typename SCALAR_T, size_t DIMENSION, typename NATIVE_T >
inline SUBCLASS_T&
protovect< SUBCLASS_T, SCALAR_T, DIMENSION, NATIVE_T >::add(
	const base::vect< SCALAR_T, DIMENSION, NATIVE_T >& src)
{
	return *this = protovect().add(*this, src);
}


template < typename SUBCLASS_T, typename SCALAR_T, size_t DIMENSION, typename NATIVE_T >
inline SUBCLASS_T&
protovect< SUBCLASS_T, SCALAR_T, DIMENSION, NATIVE_T >::sub(
	const base::vect< SCALAR_T, DIMENSION, NATIVE_T >& src0,
	const base::vect< SCALAR_T, DIMENSION, NATIVE_T >& src1)
{
	for (size_t i = 0; i < DIMENSION; ++i)
		this->set(i, src0[i] - src1[i]);

	return *this;
}


template < typename SUBCLASS_T, typename SCALAR_T, size_t DIMENSION, typename NATIVE_T >
inline SUBCLASS_T&
protovect< SUBCLASS_T, SCALAR_T, DIMENSION, NATIVE_T >::subr(
	const base::vect< SCALAR_T, DIMENSION, NATIVE_T >& src)
{
	return *this = protovect().sub(*this, src);
}


template < typename SUBCLASS_T, typename SCALAR_T, size_t DIMENSION, typename NATIVE_T >
inline SUBCLASS_T&
protovect< SUBCLASS_T, SCALAR_T, DIMENSION, NATIVE_T >::subl(
	const base::vect< SCALAR_T, DIMENSION, NATIVE_T >& src)
{
	return *this = protovect().sub(src, *this);
}


template < typename SUBCLASS_T, typename SCALAR_T, size_t DIMENSION, typename NATIVE_T >
inline SUBCLASS_T&
protovect< SUBCLASS_T, SCALAR_T, DIMENSION, NATIVE_T >::mul(
	const base::vect< SCALAR_T, DIMENSION, NATIVE_T >& src0,
	const base::vect< SCALAR_T, DIMENSION, NATIVE_T >& src1)
{
	for (size_t i = 0; i < DIMENSION; ++i)
		this->set(i, src0[i] * src1[i]);

	return *this;
}


template < typename SUBCLASS_T, typename SCALAR_T, size_t DIMENSION, typename NATIVE_T >
inline SUBCLASS_T&
protovect< SUBCLASS_T, SCALAR_T, DIMENSION, NATIVE_T >::mul(
	const base::vect< SCALAR_T, DIMENSION, NATIVE_T >& src)
{
	return *this = protovect().mul(*this, src);
}


template < typename SUBCLASS_T, typename SCALAR_T, size_t DIMENSION, typename NATIVE_T >
inline SUBCLASS_T&
protovect< SUBCLASS_T, SCALAR_T, DIMENSION, NATIVE_T >::mad(
	const base::vect< SCALAR_T, DIMENSION, NATIVE_T >& src0,
	const base::vect< SCALAR_T, DIMENSION, NATIVE_T >& src1,
	const base::vect< SCALAR_T, DIMENSION, NATIVE_T >& src2)
{
	for (size_t i = 0; i < DIMENSION; ++i)
		this->set(i, src0[i] * src1[i] + src2[i]);

	return *this;
}


template < typename SUBCLASS_T, typename SCALAR_T, size_t DIMENSION, typename NATIVE_T >
inline SUBCLASS_T&
protovect< SUBCLASS_T, SCALAR_T, DIMENSION, NATIVE_T >::mad(
	const base::vect< SCALAR_T, DIMENSION, NATIVE_T >& src0,
	const base::vect< SCALAR_T, DIMENSION, NATIVE_T >& src1)
{
	return *this = protovect().mad(src0, src1, *this);
}


template < typename SUBCLASS_T, typename SCALAR_T, size_t DIMENSION, typename NATIVE_T >
inline SUBCLASS_T&
protovect< SUBCLASS_T, SCALAR_T, DIMENSION, NATIVE_T >::mad(
	const base::vect< SCALAR_T, DIMENSION, NATIVE_T >& src0,
	const SCALAR_T c,
	const base::vect< SCALAR_T, DIMENSION, NATIVE_T >& src1)
{
	for (size_t i = 0; i < DIMENSION; ++i)
		this->set(i, src0[i] * c + src1[i]);

	return *this;
}


template < typename SUBCLASS_T, typename SCALAR_T, size_t DIMENSION, typename NATIVE_T >
inline SUBCLASS_T&
protovect< SUBCLASS_T, SCALAR_T, DIMENSION, NATIVE_T >::mad(
	const base::vect< SCALAR_T, DIMENSION, NATIVE_T >& src,
	const SCALAR_T c)
{
	return *this = protovect().mad(src, c, *this);
}


template < typename SUBCLASS_T, typename SCALAR_T, size_t DIMENSION, typename NATIVE_T >
inline SUBCLASS_T&
protovect< SUBCLASS_T, SCALAR_T, DIMENSION, NATIVE_T >::div(
	const base::vect< SCALAR_T, DIMENSION, NATIVE_T >& src0,
	const base::vect< SCALAR_T, DIMENSION, NATIVE_T >& src1)
{
	for (size_t i = 0; i < DIMENSION; ++i)
		this->set(i, src0[i] / src1[i]);

	return *this;
}


template < typename SUBCLASS_T, typename SCALAR_T, size_t DIMENSION, typename NATIVE_T >
inline SUBCLASS_T&
protovect< SUBCLASS_T, SCALAR_T, DIMENSION, NATIVE_T >::divr(
	const base::vect< SCALAR_T, DIMENSION, NATIVE_T >& src)
{
	return *this = protovect().div(*this, src);
}


template < typename SUBCLASS_T, typename SCALAR_T, size_t DIMENSION, typename NATIVE_T >
inline SUBCLASS_T&
protovect< SUBCLASS_T, SCALAR_T, DIMENSION, NATIVE_T >::divl(
	const base::vect< SCALAR_T, DIMENSION, NATIVE_T >& src)
{
	return *this = protovect().div(src, *this);
}


template < typename SUBCLASS_T, typename SCALAR_T, size_t DIMENSION, typename NATIVE_T >
inline SUBCLASS_T&
protovect< SUBCLASS_T, SCALAR_T, DIMENSION, NATIVE_T >::wsum(
	const base::vect< SCALAR_T, DIMENSION, NATIVE_T >& src0,
	const base::vect< SCALAR_T, DIMENSION, NATIVE_T >& src1,
	const SCALAR_T factor0,
	const SCALAR_T factor1)
{
	for (size_t i = 0; i < DIMENSION; ++i)
		this->set(i, src0[i] * factor0 + src1[i] * factor1);

	return *this;
}


template < typename SUBCLASS_T, typename SCALAR_T, size_t DIMENSION, typename NATIVE_T >
inline SUBCLASS_T&
protovect< SUBCLASS_T, SCALAR_T, DIMENSION, NATIVE_T >::wsum(
	const base::vect< SCALAR_T, DIMENSION, NATIVE_T >& src,
	const SCALAR_T factor0,
	const SCALAR_T factor1)
{
	return *this = protovect().wsum(*this, src, factor0, factor1);
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

	vect_generic()
	{
	}

	explicit vect_generic(
		const SCALAR_T (& src)[DIMENSION]);
};


template < typename SCALAR_T, size_t DIMENSION, typename NATIVE_T >
inline vect_generic< SCALAR_T, DIMENSION, NATIVE_T >::vect_generic(
	const SCALAR_T (& src)[DIMENSION])
: protovect< vect_generic, SCALAR_T, DIMENSION, NATIVE_T >(src)
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

	vectz()
	{
	}

	explicit vectz(
		const int (& src)[DIMENSION]);

	operator SUBCLASS_T&();

	operator const SUBCLASS_T&() const;
};


template < typename SUBCLASS_T, size_t DIMENSION, typename NATIVE_T >
inline vectz< SUBCLASS_T, DIMENSION, NATIVE_T >::vectz(
	const int (& src)[DIMENSION])
: protovect< SUBCLASS_T, int32_t, DIMENSION, NATIVE_T >(src)
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

	ivect()
	{
	}

	explicit ivect(
		const int (& src)[DIMENSION]);
};


template < size_t DIMENSION, typename NATIVE_T >
inline ivect< DIMENSION, NATIVE_T >::ivect(
	const int (&src)[DIMENSION])
: vectz< ivect, DIMENSION, NATIVE_T >(src)
{
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// class ivect<2>
// specialization of ivect for DIMENSION = 2
////////////////////////////////////////////////////////////////////////////////////////////////////

template < typename NATIVE_T >
class ivect< 2, NATIVE_T > : public vectz< ivect< 2, NATIVE_T >, 2, NATIVE_T >
{
public:

	ivect()
	{
	}

	explicit ivect(
		const int (& src)[2]);

	explicit ivect(
		const int c0,
		const int c1);
};


template < typename NATIVE_T >
inline ivect< 2, NATIVE_T >::ivect(
	const int (& src)[2])
: vectz< ivect, 2, NATIVE_T >(src)
{
}


template < typename NATIVE_T >
inline ivect< 2, NATIVE_T >::ivect(
	const int c0,
	const int c1)
{
	this->set(0, c0);
	this->set(1, c1);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// class ivect<3>
// specialization of ivect for DIMENSION = 3
////////////////////////////////////////////////////////////////////////////////////////////////////

template < typename NATIVE_T >
class ivect< 3, NATIVE_T > : public vectz< ivect< 3, NATIVE_T >, 3, NATIVE_T >
{
public:

	ivect()
	{
	}

	explicit ivect(
		const int (& src)[3]);

	explicit ivect(
		const int c0,
		const int c1,
		const int c2);
};


template < typename NATIVE_T >
inline ivect< 3, NATIVE_T >::ivect(
	const int (& src)[3])
: vectz< ivect, 3, NATIVE_T >(src)
{
}


template < typename NATIVE_T >
inline ivect< 3, NATIVE_T >::ivect(
	const int c0,
	const int c1,
	const int c2)
{
	this->set(0, c0);
	this->set(1, c1);
	this->set(2, c2);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// class ivect<4>
// specialization of ivect for DIMENSION = 4
////////////////////////////////////////////////////////////////////////////////////////////////////

template < typename NATIVE_T >
class ivect< 4, NATIVE_T > : public vectz< ivect< 4, NATIVE_T >, 4, NATIVE_T >
{
public:

	ivect()
	{
	}

	explicit ivect(
		const int (& src)[4]);

	explicit ivect(
		const int c0,
		const int c1,
		const int c2,
		const int c3);
};


template < typename NATIVE_T >
inline ivect< 4, NATIVE_T >::ivect(
	const int (& src)[4])
: vectz< ivect, 4, NATIVE_T >(src)
{
}


template < typename NATIVE_T >
inline ivect< 4, NATIVE_T >::ivect(
	const int c0,
	const int c1,
	const int c2,
	const int c3)
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

	vectr()
	{
	}

	explicit vectr(
		const float (& src)[DIMENSION]);

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
	const float (& src)[DIMENSION])
: protovect< SUBCLASS_T, float, DIMENSION, NATIVE_T >(src)
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
	return this->dot(*this);
}


template < typename SUBCLASS_T, size_t DIMENSION, typename NATIVE_T >
inline float
vectr< SUBCLASS_T, DIMENSION, NATIVE_T >::norm() const
{
	return sqrt(this->sqr());
}


template < typename SUBCLASS_T, size_t DIMENSION, typename NATIVE_T >
inline SUBCLASS_T&
vectr< SUBCLASS_T, DIMENSION, NATIVE_T >::normalise(
	const base::vect< float, DIMENSION, NATIVE_T >& src)
{
	return this->mul(src, 1.f / static_cast< const vectr& >(src).norm());
}


template < typename SUBCLASS_T, size_t DIMENSION, typename NATIVE_T >
inline SUBCLASS_T&
vectr< SUBCLASS_T, DIMENSION, NATIVE_T >::normalise()
{
	return *this = vectr().normalise(*this);
}


template < typename SUBCLASS_T, size_t DIMENSION, typename NATIVE_T >
inline SUBCLASS_T&
vectr< SUBCLASS_T, DIMENSION, NATIVE_T >::rcp(
	const base::vect< float, DIMENSION, NATIVE_T >& src)
{
	for (size_t i = 0; i < DIMENSION; ++i)
		this->set(i, 1.f / src[i]);

	return *this;
}


template < typename SUBCLASS_T, size_t DIMENSION, typename NATIVE_T >
inline SUBCLASS_T& 
vectr< SUBCLASS_T, DIMENSION, NATIVE_T >::rcp()
{
	return *this = vectr().rcp(*this);
}


template < typename SUBCLASS_T, size_t DIMENSION, typename NATIVE_T >
inline SUBCLASS_T&
vectr< SUBCLASS_T, DIMENSION, NATIVE_T >::mul(
	const base::vect< float, DIMENSION, NATIVE_T >& src,
	const base::matx< float, DIMENSION, NATIVE_T >& op)
{
	this->mul(op[0], src[0]);

	for (size_t i = 1; i < DIMENSION; ++i)
		this->mad(op[i], src[i]);

	return *this;
}


template < typename SUBCLASS_T, size_t DIMENSION, typename NATIVE_T >
inline SUBCLASS_T&
vectr< SUBCLASS_T, DIMENSION, NATIVE_T >::mul(
	const base::matx< float, DIMENSION, NATIVE_T >& op)
{
	return *this = vectr().mul(*this, op);
}


template < typename SUBCLASS_T, size_t DIMENSION, typename NATIVE_T >
inline SUBCLASS_T&
vectr< SUBCLASS_T, DIMENSION, NATIVE_T >::mulH(
	const base::vect< float, DIMENSION, NATIVE_T >& src,
	const base::matx< float, DIMENSION, NATIVE_T >& op)
{
	assert(1.f == src[DIMENSION - 1]);

	*this = op[DIMENSION - 1];

	for (size_t i = 0; i < DIMENSION - 1; ++i)
		this->mad(op[i], src[i]);

	return *this;
}


template < typename SUBCLASS_T, size_t DIMENSION, typename NATIVE_T >
inline SUBCLASS_T&
vectr< SUBCLASS_T, DIMENSION, NATIVE_T >::mulH(
	const base::matx< float, DIMENSION, NATIVE_T >& op)
{
	return *this = vectr().mulH(*this, op);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// class vect
// convenience wrapper of class vectr
////////////////////////////////////////////////////////////////////////////////////////////////////

template < size_t DIMENSION, typename NATIVE_T >
class vect : public vectr< vect< DIMENSION, NATIVE_T >, DIMENSION, NATIVE_T >
{
public:

	vect()
	{
	}

	explicit vect(
		const float (& src)[DIMENSION]);
};


template < size_t DIMENSION, typename NATIVE_T >
inline vect< DIMENSION, NATIVE_T >::vect(
	const float (& src)[DIMENSION])
: vectr< vect, DIMENSION, NATIVE_T >(src)
{
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// class vect<2>
// specialization of vect for DIMENSION = 2
////////////////////////////////////////////////////////////////////////////////////////////////////

template < typename NATIVE_T >
class vect< 2, NATIVE_T > : public vectr< vect< 2, NATIVE_T >, 2, NATIVE_T >
{
public:

	vect()
	{
	}

	explicit vect(
		const float (& src)[2]);

	explicit vect(
		const float c0,
		const float c1);

	// cross-product of this and argument, splat across this
	vect& crossr(
		const base::vect< float, 2, NATIVE_T >& src);

	// cross-product of arguments, splat across this
	vect& cross(
		const base::vect< float, 2, NATIVE_T >& src0,
		const base::vect< float, 2, NATIVE_T >& src1);
};


template < typename NATIVE_T >
inline vect< 2, NATIVE_T >::vect(
	const float (& src)[2])
: vectr< vect, 2, NATIVE_T >(src)
{
}


template < typename NATIVE_T >
inline vect< 2, NATIVE_T >::vect(
	const float c0,
	const float c1)
{
	this->set(0, c0);
	this->set(1, c1);
}


template < typename NATIVE_T >
inline vect< 2, NATIVE_T >&
vect< 2, NATIVE_T >::cross(
	const base::vect< float, 2, NATIVE_T >& src0,
	const base::vect< float, 2, NATIVE_T >& src1)
{
	const float c = src0[0] * src1[1] - src1[0] * src0[1];

	return *this = vect(c, c);
}


template < typename NATIVE_T >
inline vect< 2, NATIVE_T >&
vect< 2, NATIVE_T >::crossr(
	const base::vect< float, 2, NATIVE_T >& src)
{
	return this->cross(*this, src);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// class vect<3>
// specialization of vect for DIMENSION = 3
////////////////////////////////////////////////////////////////////////////////////////////////////

template < typename NATIVE_T >
class vect< 3, NATIVE_T > : public vectr< vect< 3, NATIVE_T >, 3, NATIVE_T >
{
public:

	vect()
	{
	}

	explicit vect(
		const float (& src)[3]);

	explicit vect(
		const float c0,
		const float c1,
		const float c2);

	// cross-product of this and argument
	vect& crossr(
		const base::vect< float, 3, NATIVE_T >& src);

	// cross-product of arguments
	vect& cross(
		const base::vect< float, 3, NATIVE_T >& src0,
		const base::vect< float, 3, NATIVE_T >& src1);
};


template < typename NATIVE_T >
inline vect< 3, NATIVE_T >::vect(
	const float (& src)[3])
: vectr< vect, 3, NATIVE_T >(src)
{
}


template < typename NATIVE_T >
inline vect< 3, NATIVE_T >::vect(
	const float c0,
	const float c1,
	const float c2)
{
	this->set(0, c0);
	this->set(1, c1);
	this->set(2, c2);
}


template < typename NATIVE_T >
inline vect< 3, NATIVE_T >&
vect< 3, NATIVE_T >::cross(
	const base::vect< float, 3, NATIVE_T >& src0,
	const base::vect< float, 3, NATIVE_T >& src1)
{
	return *this = vect(
		src0[1] * src1[2] - src1[1] * src0[2],
		src0[2] * src1[0] - src1[2] * src0[0],
		src0[0] * src1[1] - src1[0] * src0[1]);
}


template < typename NATIVE_T >
inline vect< 3, NATIVE_T >&
vect< 3, NATIVE_T >::crossr(
	const base::vect< float, 3, NATIVE_T >& src)
{
	return *this = vect().cross(*this, src);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// class vect<4>
// specialization of vect for DIMENSION = 4
////////////////////////////////////////////////////////////////////////////////////////////////////

template < typename NATIVE_T >
class vect< 4, NATIVE_T > : public vectr< vect< 4, NATIVE_T >, 4, NATIVE_T >
{
public:

	vect()
	{
	}

	explicit vect(
		const float (& src)[4]);

	explicit vect(
		const float c0,
		const float c1,
		const float c2,
		const float c3);

	// cross-product of this and argument, taken as 3-component
	vect& crossr(
		const base::vect< float, 4, NATIVE_T >& src);

	// cross-product of arguments, taken as 3-component
	vect& cross(
		const base::vect< float, 4, NATIVE_T >& src0,
		const base::vect< float, 4, NATIVE_T >& src1);
};


template < typename NATIVE_T >
inline vect< 4, NATIVE_T >::vect(
	const float (& src)[4])
: vectr< vect, 4, NATIVE_T >(src)
{
}


template < typename NATIVE_T >
inline vect< 4, NATIVE_T >::vect(
	const float c0,
	const float c1,
	const float c2,
	const float c3)
{
	this->set(0, c0);
	this->set(1, c1);
	this->set(2, c2);
	this->set(3, c3);
}


template < typename NATIVE_T >
inline vect< 4, NATIVE_T >&
vect< 4, NATIVE_T >::cross(
	const base::vect< float, 4, NATIVE_T >& src0,
	const base::vect< float, 4, NATIVE_T >& src1)
{
	return *this = vect(
		src0[1] * src1[2] - src1[1] * src0[2],
		src0[2] * src1[0] - src1[2] * src0[0],
		src0[0] * src1[1] - src1[0] * src0[1], 0.f);
}


template < typename NATIVE_T >
inline vect< 4, NATIVE_T >&
vect< 4, NATIVE_T >::crossr(
	const base::vect< float, 4, NATIVE_T >& src)
{
	return *this = vect().cross(*this, src);
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

	hamilton()
	{
	}

	explicit hamilton(
		const float (& src)[4]);

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
	const float (& src)[4])
: vectr< hamilton, 4, NATIVE_T >(src)
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

	typedef SUBCLASS_T subclass;

	protomatx()
	{
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
	for (size_t i = 0; i < DIMENSION; ++i)
		for (size_t j = 0; j < DIMENSION; ++j)
			this->set(i, j, src[i][j]);
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
	for (size_t i = 0; i < DIMENSION; ++i)
		for (size_t j = 0; j < DIMENSION; ++j)
			this->set(j, i, src[i][j]);

	return *this;
}


template < typename SUBCLASS_T, size_t DIMENSION, typename NATIVE_T >
inline SUBCLASS_T&
protomatx< SUBCLASS_T, DIMENSION, NATIVE_T >::transpose()
{
	return *this = protomatx().transpose(*this);
}


template < typename SUBCLASS_T, size_t DIMENSION, typename NATIVE_T >
inline SUBCLASS_T&
protomatx< SUBCLASS_T, DIMENSION, NATIVE_T >::mul(
	const base::matx< float, DIMENSION, NATIVE_T >& src0,
	const base::matx< float, DIMENSION, NATIVE_T >& src1)
{
	for (size_t i = 0; i < DIMENSION; ++i)
		this->set(i, vect< DIMENSION, NATIVE_T >().mul(src0[i], src1));

	return *this;
}


template < typename SUBCLASS_T, size_t DIMENSION, typename NATIVE_T >
inline SUBCLASS_T&
protomatx< SUBCLASS_T, DIMENSION, NATIVE_T >::mulr(
	const base::matx< float, DIMENSION, NATIVE_T >& src)
{
	return *this = protomatx().mul(*this, src);
}


template < typename SUBCLASS_T, size_t DIMENSION, typename NATIVE_T >
inline SUBCLASS_T&
protomatx< SUBCLASS_T, DIMENSION, NATIVE_T >::mull(
	const base::matx< float, DIMENSION, NATIVE_T >& src)
{
	return *this = protomatx().mul(src, *this);
}


template < typename SUBCLASS_T, size_t DIMENSION, typename NATIVE_T >
inline SUBCLASS_T&
protomatx< SUBCLASS_T, DIMENSION, NATIVE_T >::mul(
	const base::matx< float, DIMENSION, NATIVE_T >& src,
	const float c)
{
	for (size_t i = 0; i < DIMENSION; ++i)
		this->set(i, vect< DIMENSION, NATIVE_T >().mul(src[i], c));

	return *this;
}


template < typename SUBCLASS_T, size_t DIMENSION, typename NATIVE_T >
inline SUBCLASS_T&
protomatx< SUBCLASS_T, DIMENSION, NATIVE_T >::mul(
	const float c)
{
	return *this = protomatx().mul(*this, c);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// class matx
// instantiatable, dimensionality-agnostic square matrix of floats
////////////////////////////////////////////////////////////////////////////////////////////////////

template < size_t DIMENSION, typename NATIVE_T >
class matx : public protomatx< matx< DIMENSION, NATIVE_T >, DIMENSION, NATIVE_T >
{
public:

	matx()
	{
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
// specialization of matx for DIMENSION = 3
////////////////////////////////////////////////////////////////////////////////////////////////////

template < typename NATIVE_T >
class matx< 3, NATIVE_T > : public protomatx< matx< 3, NATIVE_T >, 3, NATIVE_T >
{
public:

	matx()
	{
	}

	explicit matx(
		const float (& src)[3][3]);

	explicit matx(
		const float c00, const float c01, const float c02,
		const float c10, const float c11, const float c12,
		const float c20, const float c21, const float c22);

	explicit matx(
		const hamilton< NATIVE_T >& q);
};


template < typename NATIVE_T >
inline matx< 3, NATIVE_T >::matx(
	const float (& src)[3][3])
: protomatx< matx, 3, NATIVE_T >(src)
{
}


template < typename NATIVE_T >
inline matx< 3, NATIVE_T >::matx(
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


template < typename NATIVE_T >
inline matx< 3, NATIVE_T >::matx(
	const hamilton< NATIVE_T >& q)
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

////////////////////////////////////////////////////////////////////////////////////////////////////
// class matx<4>
// specialization of matx for DIMENSION = 4
////////////////////////////////////////////////////////////////////////////////////////////////////

template < typename NATIVE_T >
class matx< 4, NATIVE_T > : public protomatx< matx< 4, NATIVE_T >, 4, NATIVE_T >
{
public:

	matx()
	{
	}

	explicit matx(
		const float (& src)[4][4]);

	explicit matx(
		const float c00, const float c01, const float c02, const float c03,
		const float c10, const float c11, const float c12, const float c13,
		const float c20, const float c21, const float c22, const float c23,
		const float c30, const float c31, const float c32, const float c33);

	explicit matx(
		const hamilton< NATIVE_T >& q);

	// inverse of this
	matx& inverse();

	// inverse of argument
	matx& inverse(
		const base::matx< float, 4, NATIVE_T >& src);
};


template < typename NATIVE_T >
inline matx< 4, NATIVE_T >::matx(
	const float (& src)[4][4])
: protomatx< matx, 4, NATIVE_T >(src)
{
}


template < typename NATIVE_T >
inline matx< 4, NATIVE_T >::matx(
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


template < typename NATIVE_T >
inline matx< 4, NATIVE_T >::matx(
	const hamilton< NATIVE_T >& q)
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


template < typename NATIVE_T >
inline matx< 4, NATIVE_T >&
matx< 4, NATIVE_T >::inverse(
	const base::matx< float, 4, NATIVE_T >& src)
{
	// Cramer's Rule to compute an inverse (Intel sample code)
	float dst[16], trs[16], tmp[12];

	// transpose source matrix
	for (size_t i = 0; i < 4; ++i)
	{
		trs[i +  0] = src[i][0];
		trs[i +  4] = src[i][1];
		trs[i +  8] = src[i][2];
		trs[i + 12] = src[i][3];
	}

	// calculate pairs for first 8 elements (cofactors)
	tmp[ 0]  = trs[10] * trs[15];
	tmp[ 1]  = trs[11] * trs[14];
	tmp[ 2]  = trs[ 9] * trs[15];
	tmp[ 3]  = trs[11] * trs[13];
	tmp[ 4]  = trs[ 9] * trs[14];
	tmp[ 5]  = trs[10] * trs[13];
	tmp[ 6]  = trs[ 8] * trs[15];
	tmp[ 7]  = trs[11] * trs[12];
	tmp[ 8]  = trs[ 8] * trs[14];
	tmp[ 9]  = trs[10] * trs[12];
	tmp[10]  = trs[ 8] * trs[13];
	tmp[11]  = trs[ 9] * trs[12];

	// calculate first 8 elements (cofactors)
	dst[ 0]  = tmp[ 0] * trs[ 5] + tmp[ 3] * trs[ 6] + tmp[ 4] * trs[ 7];
	dst[ 0] -= tmp[ 1] * trs[ 5] + tmp[ 2] * trs[ 6] + tmp[ 5] * trs[ 7];
	dst[ 1]  = tmp[ 1] * trs[ 4] + tmp[ 6] * trs[ 6] + tmp[ 9] * trs[ 7];
	dst[ 1] -= tmp[ 0] * trs[ 4] + tmp[ 7] * trs[ 6] + tmp[ 8] * trs[ 7];
	dst[ 2]  = tmp[ 2] * trs[ 4] + tmp[ 7] * trs[ 5] + tmp[10] * trs[ 7];
	dst[ 2] -= tmp[ 3] * trs[ 4] + tmp[ 6] * trs[ 5] + tmp[11] * trs[ 7];
	dst[ 3]  = tmp[ 5] * trs[ 4] + tmp[ 8] * trs[ 5] + tmp[11] * trs[ 6];
	dst[ 3] -= tmp[ 4] * trs[ 4] + tmp[ 9] * trs[ 5] + tmp[10] * trs[ 6];
	dst[ 4]  = tmp[ 1] * trs[ 1] + tmp[ 2] * trs[ 2] + tmp[ 5] * trs[ 3];
	dst[ 4] -= tmp[ 0] * trs[ 1] + tmp[ 3] * trs[ 2] + tmp[ 4] * trs[ 3];
	dst[ 5]  = tmp[ 0] * trs[ 0] + tmp[ 7] * trs[ 2] + tmp[ 8] * trs[ 3];
	dst[ 5] -= tmp[ 1] * trs[ 0] + tmp[ 6] * trs[ 2] + tmp[ 9] * trs[ 3];
	dst[ 6]  = tmp[ 3] * trs[ 0] + tmp[ 6] * trs[ 1] + tmp[11] * trs[ 3];
	dst[ 6] -= tmp[ 2] * trs[ 0] + tmp[ 7] * trs[ 1] + tmp[10] * trs[ 3];
	dst[ 7]  = tmp[ 4] * trs[ 0] + tmp[ 9] * trs[ 1] + tmp[10] * trs[ 2];
	dst[ 7] -= tmp[ 5] * trs[ 0] + tmp[ 8] * trs[ 1] + tmp[11] * trs[ 2];

	// calculate pairs for second 8 elements (cofactors)
	tmp[ 0]  = trs[ 2] * trs[ 7];
	tmp[ 1]  = trs[ 3] * trs[ 6];
	tmp[ 2]  = trs[ 1] * trs[ 7];
	tmp[ 3]  = trs[ 3] * trs[ 5];
	tmp[ 4]  = trs[ 1] * trs[ 6];
	tmp[ 5]  = trs[ 2] * trs[ 5];
	tmp[ 6]  = trs[ 0] * trs[ 7];
	tmp[ 7]  = trs[ 3] * trs[ 4];
	tmp[ 8]  = trs[ 0] * trs[ 6];
	tmp[ 9]  = trs[ 2] * trs[ 4];
	tmp[10]  = trs[ 0] * trs[ 5];
	tmp[11]  = trs[ 1] * trs[ 4];

	// calculate second 8 elements (cofactors)
	dst[ 8]  = tmp[ 0] * trs[13] + tmp[ 3] * trs[14] + tmp[ 4] * trs[15];
	dst[ 8] -= tmp[ 1] * trs[13] + tmp[ 2] * trs[14] + tmp[ 5] * trs[15];
	dst[ 9]  = tmp[ 1] * trs[12] + tmp[ 6] * trs[14] + tmp[ 9] * trs[15];
	dst[ 9] -= tmp[ 0] * trs[12] + tmp[ 7] * trs[14] + tmp[ 8] * trs[15];
	dst[10]  = tmp[ 2] * trs[12] + tmp[ 7] * trs[13] + tmp[10] * trs[15];
	dst[10] -= tmp[ 3] * trs[12] + tmp[ 6] * trs[13] + tmp[11] * trs[15];
	dst[11]  = tmp[ 5] * trs[12] + tmp[ 8] * trs[13] + tmp[11] * trs[14];
	dst[11] -= tmp[ 4] * trs[12] + tmp[ 9] * trs[13] + tmp[10] * trs[14];
	dst[12]  = tmp[ 2] * trs[10] + tmp[ 5] * trs[11] + tmp[ 1] * trs[ 9];
	dst[12] -= tmp[ 4] * trs[11] + tmp[ 0] * trs[ 9] + tmp[ 3] * trs[10];
	dst[13]  = tmp[ 8] * trs[11] + tmp[ 0] * trs[ 8] + tmp[ 7] * trs[10];
	dst[13] -= tmp[ 6] * trs[10] + tmp[ 9] * trs[11] + tmp[ 1] * trs[ 8];
	dst[14]  = tmp[ 6] * trs[ 9] + tmp[11] * trs[11] + tmp[ 3] * trs[ 8];
	dst[14] -= tmp[10] * trs[11] + tmp[ 2] * trs[ 8] + tmp[ 7] * trs[ 9];
	dst[15]  = tmp[10] * trs[10] + tmp[ 4] * trs[ 8] + tmp[ 9] * trs[ 9];
	dst[15] -= tmp[ 8] * trs[ 9] + tmp[11] * trs[10] + tmp[ 5] * trs[ 8];

	// calculate the reciprocal determinant and obtain the inverse
	const float det = trs[0] * dst[0] + trs[1] * dst[1] + trs[2] * dst[2] + trs[3] * dst[3];
	const float eps = (float) 1e-15;
	assert(fabs(det) > eps); (void) eps;

	const float rcp_det = 1.f / det;

	for (size_t i = 0; i < sizeof(dst) / sizeof(dst[0]); ++i)
		dst[i] *= rcp_det;

	return *this = matx(
		dst[ 0], dst[ 1], dst[ 2], dst[ 3],
		dst[ 4], dst[ 5], dst[ 6], dst[ 7],
		dst[ 8], dst[ 9], dst[10], dst[11],
		dst[12], dst[13], dst[14], dst[15]);
}


template < typename NATIVE_T >
inline matx< 4, NATIVE_T >&
matx< 4, NATIVE_T >::inverse()
{
	return *this = matx().inverse(*this);
}

} // namespace scal

#endif // scal_vect_H__
