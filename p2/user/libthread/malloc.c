/** @file malloc.c
 *  @brief thread safe wrappers for the malloc family of functions using
 *         mutexes.
 *
 *  @author Nicklaus Choo (nchoo)
 */

#include <stdlib.h>
#include <types.h>
#include <mutex.h> /* mutex_lock(), mutex_unlock() */
#include <assert.h> /* assert() */
#include <simics.h>

extern mutex_t malloc_mutex;

/** @brief Perform dynamic memory allocation on the heap.
 *  @param size Size of allocation to perform
 *
 *  @return Pointer to allocated memory on success, NULL on failure
 *  */
void *malloc(size_t __size)
{
	MAGIC_BREAK;
	mutex_lock(&malloc_mutex);
	void *p = _malloc(__size);
	mutex_unlock(&malloc_mutex);
	return p;
}

/** @brief Perform dynamic memory allocation on the heap.
 *  @param nelt Number of elements to allocate (contiguously)
 *  @param eltsize Size of each element, in bytes
 *
 *  @return Pointer to allocated memory on success, NULL on failure
 *  */
void *calloc(size_t __nelt, size_t __eltsize)
{
	MAGIC_BREAK;
	mutex_lock(&malloc_mutex);
	void *p = _calloc(__nelt, __eltsize);
	mutex_unlock(&malloc_mutex);
	return p;
}

/** @brief Reallocate a memory on the heap.
 *  @param buf Pointer of memory allocated through prior malloc call
 *  @param new_size Size of new array
 *
 *  @return Pointer to allocated memory on success, NULL on failure
 *  */
void *realloc(void *__buf, size_t __new_size)
{
	MAGIC_BREAK;
	mutex_lock(&malloc_mutex);
	void *p = realloc(__buf, __new_size);
	mutex_unlock(&malloc_mutex);
	return p;
}

/** @brief Free memory on the heap.
 *  @param buf Pointer of memory allocated through prior malloc call
 *
 *  @return Void
 *  */
void free(void *__buf)
{
	MAGIC_BREAK;
	mutex_lock(&malloc_mutex);
	_free(__buf);
	mutex_unlock(&malloc_mutex);
	return;
}
