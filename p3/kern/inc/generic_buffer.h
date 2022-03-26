/** @file generic_buffer.h
 *  @brief A buffer that can be used in consumer producer model where producers
 *         can add to the buffer and consumers can remove from the buffer.
 *
 *  @author Nicklaus Choo (nchoo)
 */

void buffer_add( void *elem );
void *buffer_rem( void );

#include <string.h> /* memset() */
#include <stdint.h> /* uint32_t */

#define NEW_BUF(BUF_TYPE, BUF_ELEM_TYPE, BUF_LIMIT)\
typedef struct {\
	uint32_t limit; /* 0 < limit */\
	uint32_t size; /* 0 <= size && size < limit */\
	uint32_t first; /* if first <= last so size == last - first */\
	uint32_t last; /* else last < first so size == limit - first + last */\
	BUF_ELEM_TYPE buffer[BUF_LIMIT];\
} BUF_TYPE

/** @def INIT_BUF(BUF_NAME)
 *
 *  @brief Initializes the buffer by zeroing out the buffer elements
 */
#define INIT_BUF(BUF_NAME, BUF_ELEM_TYPE, BUF_LIMIT) do\
{\
	((BUF_NAME)->limit) = BUF_LIMIT;\
	((BUF_NAME)->size) = 0;\
	((BUF_NAME)->first) = 0;\
	((BUF_NAME)->last) = 0;\
	memset(((BUF_NAME)->buffer), 0, ((BUF_NAME)->size) * sizeof(BUF_ELEM_TYPE));\
} while (0)

#define BUF_INSERT(BUF_NAME, BUF_ELEM) do\
{\
	/* Only insert into buffer if there is space to insert it */\
	if (((BUF_NAME)->size) < ((BUF_NAME)->limit)) {\
\
		/* Insert at index one after last element in array */\
		((BUF_NAME)->buffer[(BUF_NAME)->last]) = BUF_ELEM;\
\
		/* Update last index and increment size */\
		((BUF_NAME)->last) = (((BUF_NAME)->last) + 1) % ((BUF_NAME)->limit);\
		++((BUF_NAME)->size);\
	}\
} while (0)

#define BUF_EMPTY(BUF_NAME) (((BUF_NAME)->size) == 0)

#define BUF_REMOVE(BUF_NAME, ELEM_PTR) do\
{\
	affirm_msg(ELEM_PTR, "element pointer cannot be NULL");\
\
	/* Only remove if size is non-zero */\
	if (((BUF_NAME)->size > 0)) {\
		*(ELEM_PTR) = ((BUF_NAME)->buffer[(BUF_NAME)->first]);\
		((BUF_NAME)->first) = ((BUF_NAME)->first + 1) % ((BUF_NAME)->limit);\
		--((BUF_NAME)->size);\
	}\
} while (0)

/** @define IS_BUF(BUF_NAME)
 */
#define IS_BUF(BUF_NAME) do\
{\
	/* non NULL */\
	affirm((BUF_NAME));\
\
	/* size check */\
	assert(0 <= ((BUF_NAME)->size) && ((BUF_NAME)->size) < ((BUF_NAME)->limit));\
\
	/* limit check */\
	assert(0 < ((BUF_NAME)->limit));\
\
	/* first and last check */\
	if (((BUF_NAME)->first) <= ((BUF_NAME)->last)) {\
		assert(((BUF_NAME)->size) == ((BUF_NAME)->last) - ((BUF_NAME)->first));\
	} else {\
		assert(((BUF_NAME)->size) == ((BUF_NAME)->limit) - ((BUF_NAME)->first) +\
		       ((BUF_NAME)->last));\
	}\
	/* first and last in bounds check */\
	assert(0 <= ((BUF_NAME)->first) && ((BUF_NAME)->first) < ((BUF_NAME)->limit));\
	assert(0 <= ((BUF_NAME)->last) && ((BUF_NAME)->last) < ((BUF_NAME)->limit));\
} while (0)




