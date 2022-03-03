#include <stddef.h>
#include <malloc.h>

/* safe versions of malloc functions */
void *malloc(size_t size)
{
  (void)size;
  return NULL;
}

void *memalign(size_t alignment, size_t size)
{
  (void)alignment;
  (void)size;
  return NULL;
}

void *calloc(size_t nelt, size_t eltsize)
{
  (void)nelt;
  (void)eltsize;
  return NULL;
}

void *realloc(void *buf, size_t new_size)
{
  (void)buf;
  (void)new_size;
  return NULL;
}

void free(void *buf)
{
  (void)buf;
  return;
}

void *smalloc(size_t size)
{
  (void)size;
  return NULL;
}

void *smemalign(size_t alignment, size_t size)
{
  (void)alignment;
  (void)size;
  return NULL;
}

void sfree(void *buf, size_t size)
{
  (void)buf;
  (void)size;
  return;
}


