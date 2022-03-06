/* The 15-410 kernel project
 *
 *     buffer.h
 *
 * Structure definitions, #defines, and function prototypes
 * for a produce consumer buffer.
 */

#ifndef _BUFFER_H
#define _BUFFER_H


/* --- Prototypes --- */
/** @brief Implementation specific struct definining what a buffer_t is. Can
 *         be changed if implementation details are changed.
 */
typedef struct {
	int elem_size /* size of each element in bytes */
	int size; /* 0 <= size && size < limit */
	int limit; /* 0 < limit */
	int first; /* if first <= last, then size == last - first */
	int last; /* else last < first, then size == limit - first + last */
	void **data;
} bufer_t;

int is_buffer( buffer_t *bufp );
buffer_t *init_buffer( buffer_t *bufp, int limit, void **data );
void buffer_add( buffer_t *bufp, void *elem );
void *buffer_rem( buffer_t *bufp );
int buffer_empty( buffer_t *bufp );

/*
 * Declare your buffer prototypes here.
 */

#endif /* _LOADER_H */
