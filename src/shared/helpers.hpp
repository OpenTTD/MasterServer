/* $Id$ */

#ifndef HELPERS_HPP
#define HELPERS_HPP

/** @file helpers.hpp */
#include "macros.h"

#ifdef __cplusplus

/** When allocating using malloc/calloc in C++ it is usually needed to cast the return value
*  from void* to the proper pointer type. Another alternative would be MallocT<> as follows */
template <typename T> FORCEINLINE T* MallocT(size_t num_elements)
{
	T *t_ptr = (T*)malloc(num_elements * sizeof(T));
	return t_ptr;
}
/** When allocating using malloc/calloc in C++ it is usually needed to cast the return value
*  from void* to the proper pointer type. Another alternative would be MallocT<> as follows */
template <typename T> FORCEINLINE T* CallocT(size_t num_elements)
{
	T *t_ptr = (T*)calloc(num_elements, sizeof(T));
	return t_ptr;
}
/** When allocating using malloc/calloc in C++ it is usually needed to cast the return value
*  from void* to the proper pointer type. Another alternative would be MallocT<> as follows */
template <typename T> FORCEINLINE T* ReallocT(T* t_ptr, size_t num_elements)
{
	t_ptr = (T*)realloc(t_ptr, num_elements * sizeof(T));
	return t_ptr;
}

/** type safe swap operation */
template <typename T> void SwapT(T *a, T *b);

template <typename T> FORCEINLINE void SwapT(T *a, T *b)
{
	T t = *a;
	*a = *b;
	*b = t;
}

#endif  // __cplusplus

#endif /* HELPERS_HPP */
