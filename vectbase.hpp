#ifndef vect_base_H__
#define vect_base_H__

#include <cassert>

namespace base {

template < bool >
struct compile_assert;


template <>
struct compile_assert< true > {
	compile_assert() {
	}
};

template < typename SCALAR_T, size_t NATIVE_COUNT, typename NATIVE_T, bool = sizeof(SCALAR_T) == sizeof(NATIVE_T) >
struct alt;

template < typename SCALAR_T, size_t NATIVE_COUNT, typename NATIVE_T >
struct alt< SCALAR_T, NATIVE_COUNT, NATIVE_T, true > {
	static void scalar_mutator(
		NATIVE_T (& n)[NATIVE_COUNT], const size_t i, const SCALAR_T c) {

		n[i] = c;
	}
	static SCALAR_T scalar_accessor(
		const NATIVE_T (& n)[NATIVE_COUNT], const size_t i) {

		return n[i];
	}
};


template < typename SCALAR_T, size_t NATIVE_COUNT, typename NATIVE_T >
struct alt< SCALAR_T, NATIVE_COUNT, NATIVE_T, false > {
	static void scalar_mutator(
		NATIVE_T (& n)[NATIVE_COUNT], const size_t i, const SCALAR_T c) {

		n[i / (sizeof(NATIVE_T) / sizeof(SCALAR_T))][i % (sizeof(NATIVE_T) / sizeof(SCALAR_T))] = c;
	}
	static SCALAR_T scalar_accessor(
		const NATIVE_T (& n)[NATIVE_COUNT], const size_t i) {

		return n[i / (sizeof(NATIVE_T) / sizeof(SCALAR_T))][i % (sizeof(NATIVE_T) / sizeof(SCALAR_T))];
	}
};


template < typename SCALAR_T, size_t DIMENSION, typename NATIVE_T = SCALAR_T >
class vect {
public:

	typedef SCALAR_T scalar_t;
	typedef NATIVE_T native_t;

	enum {
		dimension = DIMENSION,
		native_count = (sizeof(SCALAR_T) * DIMENSION + sizeof(NATIVE_T) - 1) / sizeof(NATIVE_T),
		native_dimension = native_count * sizeof(NATIVE_T) / sizeof(SCALAR_T),
		scalars_per_native = sizeof(NATIVE_T) / sizeof(SCALAR_T)
	};

	vect() {
	}

	// element mutator
	void set(
		const size_t i,
		const SCALAR_T c);

	// element accessor
	SCALAR_T get(
		const size_t i) const;

	// element accessor, array subscript
	SCALAR_T operator [](
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
		const NATIVE_T src);

	// native vector accessor
	NATIVE_T getn(
		const size_t i = 0) const;

protected:

	// native element mutator - can access all scalars in the native
	// vector type, including those beyond the specified dimension
	void set_native(
		const size_t i,
		const SCALAR_T c);

private:
	NATIVE_T n[native_count];
};


template < typename SCALAR_T, size_t DIMENSION, typename NATIVE_T >
inline void
vect< SCALAR_T, DIMENSION, NATIVE_T >::set(
	const size_t i,
	const SCALAR_T c)
{
	assert(i < dimension);

	alt< scalar_t, native_count, native_t >::scalar_mutator(n, i, c);
}


template < typename SCALAR_T, size_t DIMENSION, typename NATIVE_T >
inline void
vect< SCALAR_T, DIMENSION, NATIVE_T >::set_native(
	const size_t i,
	const SCALAR_T c)
{
	assert(i < native_dimension);

	alt< scalar_t, native_count, native_t >::scalar_mutator(n, i, c);
}


template < typename SCALAR_T, size_t DIMENSION, typename NATIVE_T >
inline SCALAR_T
vect< SCALAR_T, DIMENSION, NATIVE_T >::get(
	const size_t i) const
{
	assert(i < dimension);

	return alt< scalar_t, native_count, native_t >::scalar_accessor(n, i);
}


template < typename SCALAR_T, size_t DIMENSION, typename NATIVE_T >
inline SCALAR_T
vect< SCALAR_T, DIMENSION, NATIVE_T >::operator [](
	const size_t i) const
{
	return this->get(i);
}


template < typename SCALAR_T, size_t DIMENSION, typename NATIVE_T >
inline vect< SCALAR_T, DIMENSION, NATIVE_T >&
vect< SCALAR_T, DIMENSION, NATIVE_T >::setn(
	const size_t i,
	const NATIVE_T src)
{
	assert(i < native_count);

	n[i] = src;

	return *this;
}


template < typename SCALAR_T, size_t DIMENSION, typename NATIVE_T >
inline NATIVE_T
vect< SCALAR_T, DIMENSION, NATIVE_T >::getn(
	const size_t i) const
{
	assert(i < native_count);

	return n[i];
}


template < typename SCALAR_T, size_t DIMENSION, typename NATIVE_T >
inline bool
vect< SCALAR_T, DIMENSION, NATIVE_T >::operator ==(
	const vect< SCALAR_T, DIMENSION, NATIVE_T >& src) const
{
	for (size_t i = 0; i < DIMENSION; ++i)
		if (this->operator [](i) != src[i])
			return false;

	return true;
}


template < typename SCALAR_T, size_t DIMENSION, typename NATIVE_T >
inline bool
vect< SCALAR_T, DIMENSION, NATIVE_T >::operator !=(
	const vect< SCALAR_T, DIMENSION, NATIVE_T >& src) const
{
	return !this->operator ==(src);
}


template < typename SCALAR_T, size_t DIMENSION, typename NATIVE_T >
inline bool
vect< SCALAR_T, DIMENSION, NATIVE_T >::operator >(
	const vect< SCALAR_T, DIMENSION, NATIVE_T >& src) const
{
	for (size_t i = 0; i < DIMENSION; ++i)
		if (this->operator [](i) <= src[i])
			return false;

	return true;
}


template < typename SCALAR_T, size_t DIMENSION, typename NATIVE_T >
inline bool
vect< SCALAR_T, DIMENSION, NATIVE_T >::operator >=(
	const vect< SCALAR_T, DIMENSION, NATIVE_T >& src) const
{
	for (size_t i = 0; i < DIMENSION; ++i)
		if (this->operator [](i) < src[i])
			return false;

	return true;
}


template < typename SCALAR_T, size_t DIMENSION, typename NATIVE_T = SCALAR_T >
class matx {
	vect< SCALAR_T, DIMENSION, NATIVE_T > m[DIMENSION];

public:

	typedef SCALAR_T scalar_t;
	typedef NATIVE_T native_t;

	enum { dimension = DIMENSION };

	matx() {
	}

	// element mutator
	void set(
		const size_t i,
		const size_t j,
		const SCALAR_T c);

	// element accessor
	SCALAR_T get(
		const size_t i,
		const size_t j) const;

	// row mutator
	void set(
		const size_t i,
		const vect< SCALAR_T, DIMENSION, NATIVE_T >& row);

	// row accessor
	const vect< SCALAR_T, DIMENSION, NATIVE_T >& get(
		const size_t i) const;

	// row accessor, array subscript
	const vect< SCALAR_T, DIMENSION, NATIVE_T >& operator [](
		const size_t i) const;

	bool operator ==(
		const matx& src) const;

	bool operator !=(
		const matx& src) const;

	// native mutator
	void setn(
		const size_t i,
		const size_t j,
		const NATIVE_T native);

	// native accessor
	NATIVE_T getn(
		const size_t i,
		const size_t j) const;
};


template < typename SCALAR_T, size_t DIMENSION, typename NATIVE_T >
inline void
matx< SCALAR_T, DIMENSION, NATIVE_T >::set(
	const size_t i,
	const size_t j,
	const SCALAR_T c)
{
	assert(i < dimension && j < dimension);

	m[i].set(j, c);
}


template < typename SCALAR_T, size_t DIMENSION, typename NATIVE_T >
inline SCALAR_T
matx< SCALAR_T, DIMENSION, NATIVE_T >::get(
	const size_t i,
	const size_t j) const
{
	assert(i < dimension && j < dimension);

	return m[i].get(j);
}


template < typename SCALAR_T, size_t DIMENSION, typename NATIVE_T >
inline void
matx< SCALAR_T, DIMENSION, NATIVE_T >::set(
	const size_t i,
	const vect< SCALAR_T, DIMENSION, NATIVE_T >& row)
{
	assert(i < dimension);

	m[i] = row;
}


template < typename SCALAR_T, size_t DIMENSION, typename NATIVE_T >
inline const vect< SCALAR_T, DIMENSION, NATIVE_T >&
matx< SCALAR_T, DIMENSION, NATIVE_T >::get(
	const size_t i) const
{
	assert(i < dimension);

	return m[i];
}


template < typename SCALAR_T, size_t DIMENSION, typename NATIVE_T >
inline const vect< SCALAR_T, DIMENSION, NATIVE_T >&
matx< SCALAR_T, DIMENSION, NATIVE_T >::operator [](
	const size_t i) const
{
	return this->get(i);
}


template < typename SCALAR_T, size_t DIMENSION, typename NATIVE_T >
inline bool
matx< SCALAR_T, DIMENSION, NATIVE_T >::operator ==(
	const matx< SCALAR_T, DIMENSION, NATIVE_T >& src) const
{
	for (size_t i = 0; i < dimension; ++i)
		if (this->operator [](i) != src[i])
			return false;

	return true;
}


template < typename SCALAR_T, size_t DIMENSION, typename NATIVE_T >
inline bool
matx< SCALAR_T, DIMENSION, NATIVE_T >::operator !=(
	const matx< SCALAR_T, DIMENSION, NATIVE_T >& src) const
{
	return !this->operator ==(src);
}


template < typename SCALAR_T, size_t DIMENSION, typename NATIVE_T >
inline void
matx< SCALAR_T, DIMENSION, NATIVE_T >::setn(
	const size_t i,
	const size_t j,
	const NATIVE_T n)
{
	assert(i < DIMENSION && j < m[i].native_count);

	m[i].setn(j, n);
}


template < typename SCALAR_T, size_t DIMENSION, typename NATIVE_T >
inline NATIVE_T
matx< SCALAR_T, DIMENSION, NATIVE_T >::getn(
	const size_t i,
	const size_t j) const
{
	assert(i < dimension && j < m[i].native_count);

	return m[i].getn(j);
}

} // namespace base

#endif // vect_base_H__
