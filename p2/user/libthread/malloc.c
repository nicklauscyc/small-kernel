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
extern mutex_t malloc_mutex;
extern int THR_INITIALIZED;

/** @brief Perform dynamic memory allocation on the heap.
 *  @param size Size of allocation to perform
 *
 *  @return Pointer to allocated memory on success, NULL on failure
 *  */
void *malloc(size_t __size)
{
    if (THR_INITIALIZED) {
	    mutex_lock(&malloc_mutex);
    }

	void *p = _malloc(__size);

    if (THR_INITIALIZED) {
	    mutex_unlock(&malloc_mutex);
    }
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
    if (THR_INITIALIZED) {
	    mutex_lock(&malloc_mutex);
    }
	void *p = _calloc(__nelt, __eltsize);
    if (THR_INITIALIZED) {
	    mutex_unlock(&malloc_mutex);
    }
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
    if (THR_INITIALIZED) {
	    mutex_lock(&malloc_mutex);
    }
	void *p = realloc(__buf, __new_size);
    if (THR_INITIALIZED) {
	    mutex_unlock(&malloc_mutex);
    }
	return p;
}

/** @brief Free memory on the heap.
 *  @param buf Pointer of memory allocated through prior malloc call
 *
 *  @return Void
 *  */
void free(void *__buf)
{
    if (THR_INITIALIZED) {
	    mutex_lock(&malloc_mutex);
    }
	_free(__buf);
    if (THR_INITIALIZED) {
	    mutex_unlock(&malloc_mutex);
    }
	return;
}
