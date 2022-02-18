/*
 * these functions should be thread safe.
 * It is up to you to rewrite them
 * to make them thread safe.
 *
 * TODO add cvars once cvars is done
 *
 */

#include <stdlib.h>
#include <types.h>
#include <stddef.h>
#include <mutex.h>
#include <assert.h> /* assert() */
#include <simics.h> /* MAGIC_BREAK */
extern mutex_t malloc_mutex;

void *malloc(size_t __size)
{
	//assert(&malloc_mutex);

	lprintf("&malloc_mutex: %p\n", &malloc_mutex);

	mutex_lock(&malloc_mutex);
	void *p = _malloc(__size);
	mutex_unlock(&malloc_mutex);
	return p;
}

void *calloc(size_t __nelt, size_t __eltsize)
{
	//assert(&malloc_mutex);
	MAGIC_BREAK;
	lprintf("&malloc_mutex: %p\n", &malloc_mutex);
	mutex_lock(&malloc_mutex);
	void *p = _calloc(__nelt, __eltsize);
	mutex_unlock(&malloc_mutex);
	return p;
}

void *realloc(void *__buf, size_t __new_size)
{

	//assert(&malloc_mutex);
    MAGIC_BREAK;
	lprintf("&malloc_mutex: %p\n", &malloc_mutex);

	mutex_lock(&malloc_mutex);
	void *p = realloc(__buf, __new_size);
	mutex_unlock(&malloc_mutex);
	return p;
}

void free(void *__buf)
{
	MAGIC_BREAK;
	//assert(&malloc_mutex);
	mutex_lock(&malloc_mutex);
	_free(__buf);
	mutex_unlock(&malloc_mutex);
	return;
}
