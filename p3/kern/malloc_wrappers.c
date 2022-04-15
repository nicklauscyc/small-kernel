#include <malloc.h>
#include <assert.h>				/* affirm */
#include <stddef.h>				/* size_t */
#include <malloc_internal.h>	/* _malloc family of functions */
#include <lib_thread_management/mutex.h> /* mutex_t */
#include <logger.h>

/* Where to put this mutex? We could initialize it in scheduler init? */
static mutex_t malloc_mux;
static int is_mutex_init = 0;

/* These macros allow us to easily initialize the malloc mutex
 * upon the first call to the malloc library. */
#define LOCK do\
{\
	if (!is_mutex_init) {\
		mutex_init(&malloc_mux);\
		is_mutex_init = 1;\
	}\
	mutex_lock(&malloc_mux);\
} while(0)\

#define UNLOCK do\
{\
	affirm(is_mutex_init);\
	mutex_unlock(&malloc_mux);\
} while(0)\


/* safe versions of malloc functions */
void *malloc(size_t size)
{
	LOCK;
	void *p = _malloc(size);
	log("malloc returned %p", p);
	UNLOCK;
    return p;
}

void *memalign(size_t alignment, size_t size)
{
	LOCK;
	void *p = _memalign(alignment, size);
	log("memalign returned %p, size 0x%08lx", p, size);
	UNLOCK;
	return p;
}

void *calloc(size_t nelt, size_t eltsize)
{
	LOCK;
    void *p = _calloc(nelt, eltsize);
	UNLOCK;
	return p;
}

void *realloc(void *buf, size_t new_size)
{
	LOCK;
    void *p = _realloc(buf, new_size);
	UNLOCK;
	return p;
}

void free(void *buf)
{
	LOCK;
	_free(buf);
	UNLOCK;
}

void *smalloc(size_t size)
{
	LOCK;
    void *p = _smalloc(size);
	log("smalloc returned %p, size 0x%08lx", p, size);

	UNLOCK;
	return p;
}

void *smemalign(size_t alignment, size_t size)
{
	LOCK;
    void *p = _smemalign(alignment, size);
	log("smemalign returned %p, size 0x%08lx", p, size);

	UNLOCK;
	return p;
}

void sfree(void *buf, size_t size)
{
	LOCK;
	_sfree(buf, size);
	log("sfree(): freed %p, size 0x%08lx", buf, size);

	UNLOCK;
}


