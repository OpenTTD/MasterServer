/* $Id$ */

/*
 * This file is part of OpenTTD's master server/updater and content service.
 * OpenTTD is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * OpenTTD is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with OpenTTD. If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef HELPERS_HPP
#define HELPERS_HPP

/** @file helpers.hpp */

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

#endif /* HELPERS_HPP */
