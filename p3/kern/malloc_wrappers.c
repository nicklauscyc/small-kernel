/** @file malloc_wrappers.c
 *  @brief Wrappers for malloc functions ensuring they can be
 *  run while avoiding race conditions.
 *
 *  This is done by employing a global lock (which acts more like
 *  a queue). */

#include <malloc.h>
#include <assert.h>				/* affirm */
#include <stddef.h>				/* size_t */
#include <malloc_internal.h>	/* _malloc family of functions */
#include <lib_thread_management/mutex.h> /* mutex_t */
#include <logger.h>
#include <simics.h>

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


/** @brief Allocate memory
 *
 *  @param size Size of memory to allocate
 *  @return pointer to newly allocated memory*/
void *malloc(size_t size)
{
	LOCK;
	void *p = _malloc(size);
	log("malloc returned %p", p);
	UNLOCK;
    return p;
}

/** @brief Allocate aligned memory
 *
 *  @param alignment Size to align memory to
 *  @param size Size of memory to allocate
 *  @return aligned pointer to newly allocated memory */
void *memalign(size_t alignment, size_t size)
{
	LOCK;
	void *p = _memalign(alignment, size);
	log("memalign returned %p, size %u", p, size);

	UNLOCK;
	return p;
}

/** @brief Allocate memory
 *
 *  @param nelt Number of elements
 *  @param eltsize Size of each element
 *  @return pointer to newly allocated memory*/
void *calloc(size_t nelt, size_t eltsize)
{
	LOCK;
    void *p = _calloc(nelt, eltsize);
	UNLOCK;
	return p;
}

/** @brief Reallocate memory
 *
 *  @param buf Previously allocated pointer
 *  @param new_size New size of pointer
 *  @return pointer to newly allocated memory */
void *realloc(void *buf, size_t new_size)
{
	LOCK;
    void *p = _realloc(buf, new_size);
	UNLOCK;
	return p;
}

/** @brief Free memory
 *
 *  @param buf Previously allocated pointer
 *  @return Void. */
void free(void *buf)
{
	LOCK;
	_free(buf);
	log("free(): freed %p", buf);


	UNLOCK;
}

/** @brief Allocate memory
 *
 *  @param size Size of memory to allocate
 *  @return Void. */
void *smalloc(size_t size)
{
	LOCK;
    void *p = _smalloc(size);
	log("smalloc returned %p, size %u", p, size);


	UNLOCK;
	return p;
}

/** @brief Allocate aligned memory
 *
 *  @param alignment Size to align memory to
 *  @param size Size of memory to allocate
 *  @return aligned pointer to newly allocated memory */
void *smemalign(size_t alignment, size_t size)
{
	LOCK;
    void *p = _smemalign(alignment, size);
	log("smemalign returned %p, size %u", p, size);


	UNLOCK;
	return p;
}

/** @brief Free memory
 *
 *  @param buf Pointer to previously allocated memory (by smalloc or smemalign)
 *  @param size Size of memory to free
 *  @return Void. */
void sfree(void *buf, size_t size)
{
	LOCK;
	_sfree(buf, size);
	log("sfree(): freed %p, size %u", buf, size);


	UNLOCK;
}


