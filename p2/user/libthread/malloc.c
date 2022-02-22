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

void *malloc(size_t __size)
{
	mutex_lock(&malloc_mutex);
	void *p = _malloc(__size);
	mutex_unlock(&malloc_mutex);
	return p;
}

void *calloc(size_t __nelt, size_t __eltsize)
{
	mutex_lock(&malloc_mutex);
	void *p = _calloc(__nelt, __eltsize);
	mutex_unlock(&malloc_mutex);
	return p;
}

void *realloc(void *__buf, size_t __new_size)
{
	mutex_lock(&malloc_mutex);
	void *p = realloc(__buf, __new_size);
	mutex_unlock(&malloc_mutex);
	return p;
}

void free(void *__buf)
{
	mutex_lock(&malloc_mutex);
	_free(__buf);
	mutex_unlock(&malloc_mutex);
	return;
}
