/** @file buffer.c
 *  @brief Particular implementation of a buffer_t. The only other place
 *  	   where implementation specific code is written is in the buffer_t
 *  	   struct in buffer.h. All other implementation details are written
 *  	   in this file
 *
 *  @author Nicklaus Choo (nchoo)
 */

#include <string.h> /* memset() */
#include <buffer.h> /* buffer_t */
#include <assert.h> /* assert() */

/* Size of a data element in buffer */
#define ELEM_SIZE_BYTES 4

/** @brief Checks invariants for buffer_t
 *
 *  The invariants here are implemented as asserts so in the even that
 *  an assertion fails, the developer knows exactly which assertion
 *  fails. For this implementation we use a circular array.
 *
 *  @param buf Pointer to buffer to be checked
 *  @return 1 if valid buffer pointer, does not return and throws assertion
 *  	    error otherwise
 */
int
is_buffer( buffer_t *bufp )
{
	/* buf cannot be NULL */
	assert(bufp);

	/* size check */
	assert(0 <= bufp->size && bufp->size <= bufp->limit);

	/* limit check */
	assert(0 < bufp->limit);

	/* first and last check */
	if (bufp->first <= bufp->last) {
	  assert(bufp->size == bufp->last - bufp->first);
	} else {
	  assert(bufp->size == bufp->limit - bufp->first + bufp->last);
	}
	/* first and last in bounds check */
	assert(0 <= bufp->first && bufp->first < bufp->limit);
	assert(0 <= bufp->last && bufp->last < bufp->limit);

	/* data non-NULL check */
	assert(bufp->data);
	return 1;
}

/** @brief Initializes a buffer given a pointer to it.
 *
 *  Zero limit buffers are disallowed.
 *
 *  @param bufp Pointer to buffer_t to be initialized
 *  @param limit Actual size of the buffer.
 *  @param data Array used to store elements.
 *  @return 0 on success, negative value on error
 */
int
init_buffer( buffer_t *bufp, int limit, void **data, int elem_size )
{
	/* Check argument validity */
	if (!bufp) {
		return -1;
	}
	if (limit <= 0) {
		return -1;
	}
	if (!data) {
		return -1;
	}
	if (elem_size <= 0) {
		return -1;
	}
	/* Set fields */
	bufp->elem_size = elem_size;
	bufp->size = 0;
	bufp->limit = limit;
	bufp->first = 0;
	bufp->last = 0;
	memset(data, 0, elem_size * limit);
	bufp->data = data;

	return 0;
}

/** @brief Adds new element to the buffer. If the buffer is full, does
 *         nothing
 *
 *  @param bufp Pointer to buffer to add element to.
 *  @param elem Element to add to the buffer.
 *  @return Void.
 */
void
buffer_add( buffer_t *bufp, void *elem )
{
	assert(is_buffer(bufp));

	/* Do nothing if the buffer is full */
	if (bufp->size == bufp->limit) {
		return;
	}
	/* Insert at index one after last element in buffer */
	*(bufp->data + (bufp->last * bufp->elem_size)) = elem;

	/* Update last index and increment size*/
	bufp->last = (bufp->last + 1) % bufp->limit;
	bufp->size += 1;
	assert(is_buffer(bufp));
}

/** @brief Removes first character of the buffer

 *  @param bufp Pointer to buffer
 *  @return First character of the buffer
 */
void *
buffer_rem( buffer_t *bufp )
{
	assert(is_buffer(bufp));

	/* Get first element and 'remove' from the bufay, decrement size */
	void *elem = *(bufp->data + bufp->first * bufp->elem_size);
	bufp->first = (bufp->first + 1) % bufp->limit;
	bufp->size -= 1;
	assert(is_buffer(bufp));

	/* Return 'popped off' element */
	return elem;
}

/** @brief Checks if buffer is empty
 *  @param buf Pointer to buffer
 *  @return 1 if empty, 0 otherwise.
 */
int
buffer_empty( buffer_t *bufp )
{
	assert(is_buffer(bufp));
	return bufp->size == 0;
}

