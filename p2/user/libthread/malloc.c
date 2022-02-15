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
extern mutex_t *mmp;

void *malloc(size_t __size)
{
	assert(mmp);

	lprintf("mmp: %p\n", mmp);

	mutex_lock(mmp);
	void *p = _malloc(__size);
	mutex_unlock(mmp);
	return p;
}

void *calloc(size_t __nelt, size_t __eltsize)
{
	assert(mmp);
	MAGIC_BREAK;
	lprintf("mmp: %p\n", mmp);
	mutex_lock(mmp);
	void *p = _calloc(__nelt, __eltsize);
	mutex_unlock(mmp);
	return p;
}

void *realloc(void *__buf, size_t __new_size)
{

	assert(mmp);
    MAGIC_BREAK;
	lprintf("mmp: %p\n", mmp);

	mutex_lock(mmp);
	void *p = realloc(__buf, __new_size);
	mutex_unlock(mmp);
	return p;
}

void free(void *__buf)
{
	MAGIC_BREAK;
	assert(mmp);
	mutex_lock(mmp);
	_free(__buf);
	mutex_unlock(mmp);
	return;
}
