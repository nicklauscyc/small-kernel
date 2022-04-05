/** @file variable_buffer.h
 *  @brief Generalized circular buffer module for a consumer producer model
 *         where producers can add to the buffer and consumers can remove from
 *         the buffer.
 *
 *  TODO add mutex support
 *
 *  In order to use the other variable buffer functions on a particular
 *  buffer instance, the first thing one needs to do is to define the buffer
 *  type with new_buf() and then initialize that particular buffer via a pointer
 *  to it supplied to init_buf().
 *
 *  For variable buffer functions that take in pointers as arguments, due to
 *  macros being expanded inline, to avoid compiler complaints of
 *  &variable_name always evaluating to 'true', the affirm() statements will
 *  affirm(ptr_name != NULL)
 *
 *  a pointer to variable_name, or else the compiler will complain that the
 *  NULL-pointer check is unecessary. On the contrary these checks are indeed
 *  necessary because variable functions to do not know if pointers passed to
 *  them are valid or not.
 *
 *  @author Nicklaus Choo (nchoo)
 */

#ifndef _VARIABLE_BUFFER_H_
#define _VARIABLE_BUFFER_H_

#include <string.h> /* memset() */
#include <stdint.h> /* uint32_t */
#include <stddef.h> /* NULL */

/** @def new_buf
 *  @brief Generates a new structure of type BUF_TYPE, with an array of
 *         BUF_ELEM_TYPE with size BUF_LIMIT. The array is a circular array.
 *
 *  Usage: new_buf(BUF_TYPE, BUF_ELEM_TYPE, BUF_LIMIT); //create the type
 *         BUF_TYPE buffer_name; //instantiate buffer of the given type
 *
 *  @param BUF_TYPE The type of the newly generated structure
 *  @param BUF_ELEM_TYPE The type of elements in the buffer
 *  @param BUF_LIMIT Maximum capacity of the buffer
 */
#define new_buf(BUF_TYPE, BUF_ELEM_TYPE, BUF_LIMIT)\
typedef struct {\
	uint32_t limit; /* 0 < limit */\
	uint32_t size; /* 0 <= size && size < limit */\
	uint32_t first; /* if first <= last so size == last - first */\
	uint32_t last; /* else last < first so size == limit - first + last */\
	BUF_ELEM_TYPE buffer[BUF_LIMIT];\
} BUF_TYPE

/** @def init_buf(BUF_NAME, BUF_ELEM_TYPE, BUF_LIMIT)
 *  @brief Initializes the buffer by zeroing out the buffer elements and setting
 *         struct fields to correct initial values
 *
 *  Usage: init_buf(BUF_NAME, BUF_ELEM_TYPE, BUF_LIMIT);
 *
 *  @param BUF_NAME Pointer to the buffer to initialize
 *  @param BUF_ELEM_TYPE The type of elements in the buffer
 *  @param BUF_LIMIT Maximum capacity of the buffer
 */
#define init_buf(BUF_NAME, BUF_ELEM_TYPE, BUF_LIMIT) do\
{\
	/* Pointer argument checks */\
	affirm_msg((BUF_NAME) != NULL, "Variable buffer pointer cannot be NULL!");\
\
	/* Set all fields to correct values */\
	((BUF_NAME)->limit) = BUF_LIMIT;\
	((BUF_NAME)->size) = 0;\
	((BUF_NAME)->first) = 0;\
	((BUF_NAME)->last) = 0;\
\
	/* Zero out the buffer */\
	memset(((BUF_NAME)->buffer),0,((BUF_NAME)->size) * sizeof(BUF_ELEM_TYPE));\
} while (0)

/** @def buf_insert(BUF_NAME, BUF_ELEM)
 *  @brief Inserts an element into the end of the buffer
 *
 *  Usage: buf_insert(BUF_NAME, BUF_ELEM);
 *
 *  @param BUF_NAME Pointer to the buffer to insert to
 *  @param BUF_ELEM Element to insert into the buffer. Requires that this
 *         element be of correct type, else behavior is undefined
 */
#define buf_insert(BUF_NAME, BUF_ELEM) do\
{\
	/* Pointer argument checks */\
	affirm_msg((BUF_NAME) != NULL, "Variable buffer pointer cannot be NULL!");\
\
	/* Only insert into buffer if there is space to insert it */\
	if (((BUF_NAME)->size) < ((BUF_NAME)->limit)) {\
\
		/* Insert at index one after last element in array */\
		((BUF_NAME)->buffer[(BUF_NAME)->last]) = (BUF_ELEM);\
\
		/* Update last index and increment size */\
		((BUF_NAME)->last) = (((BUF_NAME)->last) + 1) % ((BUF_NAME)->limit);\
		++((BUF_NAME)->size);\
	}\
} while (0)

/** @def buf_empty(BUF_NAME)
 *  @brief Simple boolean for whether buffer is empty or not
 *
 *  Usage: buf_empty(BUF_NAME);
 *
 *  Since we are trying to do the equivalent of an affirm() statement to check
 *  for invalid NULL pointer arguments in a C macro that "returns" a value,
 *  we use the comma operator, which allows us to have multiple expressions
 *  that evaluate to the last expression.
 *
 *  In our case, the comma separated expressions have type (void, bool) and
 *  since both expression sequences need matching types, we add dummy
 *  expressions '(void) (BUF_NAME)', and simply '0' to our ternary operator
 *  'return' expressions.
 *
 *  @param BUF_NAME Pointer to the variable buffer
 */
#define buf_empty(BUF_NAME)\
(\
	/* BUF_NAME non-NULL check */\
	((BUF_NAME) != NULL) ?\
		((void) (BUF_NAME), (BUF_NAME)->size == 0) :\
		(panic("Variable buffer pointer cannot be NULL"), 0)\
)

/** @def buf_remove(BUF_NAME, ELEM_PTR)
 *  @brief Removes the first element (element added earliest compared to all
 *         other elements) in the buffer
 *
 *  Usage: buf_remove(BUF_NAME, ELEM_PTR);
 *
 *  @param BUF_NAME Variable buffer pointer
 *  @param ELEM_PTR Pointer to store buffer element
 */
#define buf_remove(BUF_NAME, ELEM_PTR) do\
{\
	/* Pointer argument checks */\
	affirm_msg((BUF_NAME) != NULL, "Variable buffer pointer cannot be NULL");\
	affirm_msg((ELEM_PTR) != NULL, "Element pointer cannot be NULL");\
\
	/* Only remove if size is non-zero */\
	if (((BUF_NAME)->size > 0)) {\
		*(ELEM_PTR) = ((BUF_NAME)->buffer[(BUF_NAME)->first]);\
		((BUF_NAME)->first) = ((BUF_NAME)->first + 1) % ((BUF_NAME)->limit);\
		--((BUF_NAME)->size);\
	}\
} while (0)

/** @define is_buf(BUF_NAME)
 * 	@brief Invariant checks for a variable buffer that throw assertion errors
 * 	       on invariant violations. Does nothing if assert()'s are off.
 *
 *  Usage: is_buf(BUF_NAME);
 *
 *  @param BUF_NAME Variable buffer pointer
 */
#define is_buf(BUF_NAME) do\
{\
	/* non NULL */\
	affirm_msg((BUF_NAME) != NULL, "Variable buffer pointer cannot be NULL!");\
\
	/* size check */\
	assert(0 <= ((BUF_NAME)->size) && ((BUF_NAME)->size) <\
	       ((BUF_NAME)->limit));\
\
	/* limit check */\
	assert(0 < ((BUF_NAME)->limit));\
\
	/* first and last check */\
	if (((BUF_NAME)->first) <= ((BUF_NAME)->last)) {\
		assert(((BUF_NAME)->size) == ((BUF_NAME)->last) -\
		       ((BUF_NAME)->first));\
	} else {\
		assert(((BUF_NAME)->size) == ((BUF_NAME)->limit) -\
		       ((BUF_NAME)->first) + ((BUF_NAME)->last));\
	}\
	/* first and last in bounds check */\
	assert(0 <= ((BUF_NAME)->first) && ((BUF_NAME)->first) <\
	       ((BUF_NAME)->limit));\
	assert(0 <= ((BUF_NAME)->last) && ((BUF_NAME)->last) <\
	       ((BUF_NAME)->limit));\
} while (0)

#endif /* _VARIABLE_BUFFER_H_ */
