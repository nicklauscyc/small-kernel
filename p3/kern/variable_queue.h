/** @file variable_queue.h
 *
 *  @brief Generalized queue module for data collection.
 *
 *  This header file is not placed in inc/ since it is private to the kernel's
 *  own modules.
 *
 *  @author Nicklaus Choo (nchoo)
 */

#ifndef _VARIABLE_QUEUE_H_
#define _VARIABLE_QUEUE_H_

#include <stddef.h> /* NULL */

/** @def Q_NEW_HEAD(Q_HEAD_TYPE, Q_ELEM_TYPE)
 *
 *  @brief Generates a new structure of type Q_HEAD_TYPE representing the head
 *  of a queue of elements of type Q_ELEM_TYPE.
 *
 *  Usage: Q_NEW_HEAD(Q_HEAD_TYPE, Q_ELEM_TYPE); //create the type <br>
 *         Q_HEAD_TYPE headName; //instantiate a head of the given type
 *
 *  This queue must only be manipulated by one thread at any one time
 *
 *  @param Q_HEAD_TYPE the type you wish the newly-generated structure to have.
 *
 *  @param Q_ELEM_TYPE the type of elements stored in the queue.
 *         Q_ELEM_TYPE must be a structure.
 *
 **/
#define Q_NEW_HEAD(Q_HEAD_TYPE, Q_ELEM_TYPE) \
typedef struct {\
	struct Q_ELEM_TYPE *front;\
	struct Q_ELEM_TYPE *tail;\
} Q_HEAD_TYPE;

/** @def Q_NEW_LINK(Q_ELEM_TYPE)
 *
 *  @brief Instantiates a link within a structure, allowing that structure to be
 *         collected into a queue created with Q_NEW_HEAD.
 *
 *  Usage: <br>
 *  typedef struct Q_ELEM_TYPE {<br>
 *  Q_NEW_LINK(Q_ELEM_TYPE) LINK_NAME; //instantiate the link <br>
 *  } Q_ELEM_TYPE; <br>
 *
 *  A structure can have more than one link defined within it, as long as they
 *  have different names. This allows the structure to be placed in more than
 *  one queue simultanteously.
 *
 *  index 0 is the previous pointer, index 1 is the next pointer
 *
 *  @param Q_ELEM_TYPE the type of the structure containing the link
 **/
#define Q_NEW_LINK(Q_ELEM_TYPE) \
struct {\
	struct Q_ELEM_TYPE *prev;\
	struct Q_ELEM_TYPE *next;\
}

/** @def Q_INIT_HEAD(Q_HEAD)
 *
 *  @brief Initializes the head of a queue so that the queue head can be used
 *         properly.
 *  @param Q_HEAD Pointer to queue head to initialize
 **/
#define Q_INIT_HEAD(Q_HEAD) do\
{\
	affirm_msg((Q_HEAD) != NULL,\
	           "Variable queue head pointer cannot be NULL!");\
	(Q_HEAD)->front = NULL;\
	(Q_HEAD)->tail = NULL;\
} while(0)

/** @def Q_INIT_ELEM(Q_ELEM, LINK_NAME)
 *
 *  @brief Initializes the link named LINK_NAME in an instance of the structure
 *         Q_ELEM.
 *
 *  Once initialized, the link can be used to organized elements in a queue.
 *
 *  @param Q_ELEM Pointer to the structure instance containing the link
 *  @param LINK_NAME The name of the link to initialize
 **/
#define Q_INIT_ELEM(Q_ELEM, LINK_NAME) do\
{\
	affirm_msg((Q_ELEM) != NULL,\
	           "Variable queue element pointer cannot be NULL!");\
	((Q_ELEM)->LINK_NAME).prev = NULL;\
	((Q_ELEM)->LINK_NAME).next = NULL;\
} while(0)

/** @def Q_INSERT_FRONT(Q_HEAD, Q_ELEM, LINK_NAME)
 *
 *  @brief Inserts the queue element pointed to by Q_ELEM at the front of the
 *         queue headed by the structure Q_HEAD.
 *
 *  The link identified by LINK_NAME will be used to organize the element and
 *  record its location in the queue. Requires that Q_INIT_ELEM() be called on
 *  Q_ELEM prior to insertion.
 *
 *  @param Q_HEAD Pointer to the head of the queue into which Q_ELEM will be
 *         inserted
 *  @param Q_ELEM Pointer to the element to insert into the queue
 *  @param LINK_NAME Name of the link used to organize the queue
 *  @return Void (you may change this if your implementation calls for a
 *                return value)
 **/
#define Q_INSERT_FRONT(Q_HEAD, Q_ELEM, LINK_NAME) do\
{\
	/* Pointer argument checks */\
	affirm_msg((Q_HEAD) != NULL,\
	           "Variable queue head pointer cannot be NULL!");\
	affirm_msg((Q_ELEM) != NULL,\
	           "Variable queue element pointer cannot be NULL!")\
\
	/* Q is currently empty, insert tail as well */\
	if ((Q_HEAD)->front == NULL) {\
		affirm_msg((Q_HEAD)->tail == NULL,\
		           "Empty variable queue head and tail must be both NULL");\
		(Q_HEAD)->front = (Q_ELEM);\
		(Q_HEAD)->tail = (Q_ELEM);\
\
	/* Q not empty, update front */\
	} else {\
		(((Q_HEAD)->front)->LINK_NAME).prev = (Q_ELEM);\
		((Q_ELEM)->LINK_NAME).next = (Q_HEAD)->front;\
		(Q_HEAD)->front = (Q_ELEM);\
	}\
} while(0)

/** @def Q_INSERT_TAIL(Q_HEAD, Q_ELEM, LINK_NAME)
 *  @brief Inserts the queue element pointed to by Q_ELEM at the end of the
 *         queue headed by the structure pointed to by Q_HEAD.
 *
 *  The link identified by LINK_NAME will be used to organize the element and
 *  record its location in the queue.
 *
 *  @param Q_HEAD Pointer to the head of the queue into which Q_ELEM will be
 *         inserted
 *  @param Q_ELEM Pointer to the element to insert into the queue
 *  @param LINK_NAME Name of the link used to organize the queue
 *
 *  @return Void (you may change this if your implementation calls for a
 *                return value)
 **/
#define Q_INSERT_TAIL(Q_HEAD, Q_ELEM, LINK_NAME) do\
{\
	/* Pointer argument checks */\
	affirm_msg((Q_HEAD) != NULL,\
	           "Variable queue head pointer cannot be NULL!");\
	affirm_msg((Q_ELEM) != NULL,\
	           "Variable queue element pointer cannot be NULL!");\
\
	/* Q is currently empty, insert front as well */\
	if ((Q_HEAD)->tail == NULL) {\
		affirm_msg((Q_HEAD)->front == NULL,\
		           "Empty variable queue head and tail must be both NULL");\
		(Q_HEAD)->front = (Q_ELEM);\
		(Q_HEAD)->tail = (Q_ELEM);\
\
	/* Q not empty, update tail */\
	} else {\
		(((Q_HEAD)->tail)->LINK_NAME).next = (Q_ELEM);\
		((Q_ELEM)->LINK_NAME).prev = (Q_HEAD)->tail;\
		(Q_HEAD)->tail = (Q_ELEM);\
	}\
} while(0)


/** @def Q_GET_FRONT(Q_HEAD)
 *
 *  @brief Returns a pointer to the first element in the queue, or NULL
 *  (memory address 0) if the queue is empty.
 *
 *  @param Q_HEAD Pointer to the head of the queue
 *  @return Pointer to the first element in the queue, or NULL if the queue
 *          is empty
 **/
#define Q_GET_FRONT(Q_HEAD)\
(\
	/* Q_HEAD non-NULL check */\
	((Q_HEAD) != NULL) ?\
		((void) (Q_HEAD), (Q_HEAD)->front) :\
		(panic("Variable queue head pointer cannot be NULL"), NULL)\
)

/** @def Q_GET_TAIL(Q_HEAD)
 *
 *  @brief Returns a pointer to the last element in the queue, or NULL
 *  (memory address 0) if the queue is empty.
 *
 *  @param Q_HEAD Pointer to the head of the queue
 *  @return Pointer to the last element in the queue, or NULL if the queue
 *          is empty
 **/
#define Q_GET_TAIL(Q_HEAD)\
(\
	/* Q_HEAD non-NULL check */\
	((Q_HEAD) != NULL) ?\
		((void) (Q_HEAD), (Q_HEAD)->tail) :\
		(panic("Variable queue head pointer cannot be NULL"), NULL)\
)

/** @def Q_GET_NEXT(Q_ELEM, LINK_NAME)
 *
 *  @brief Returns a pointer to the next element in the queue, as linked to by
 *         the link specified with LINK_NAME.
 *
 *  If Q_ELEM is not in a queue or is the last element in the queue,
 *  Q_GET_NEXT should return NULL.
 *
 *  @param Q_ELEM Pointer to the queue element before the desired element
 *  @param LINK_NAME Name of the link organizing the queue
 *
 *  @return The element after Q_ELEM, or NULL if there is no next element
 **/
#define Q_GET_NEXT(Q_ELEM, LINK_NAME)\
(\
	/* Q_ELEM non-NULL check */\
	((Q_ELEM) != NULL) ?\
		((void) (Q_ELEM), ((Q_ELEM)->LINK_NAME).next) :\
		(panic("Variable queue element pointer cannot be NULL"), NULL)\
)

/** @def Q_GET_PREV(Q_ELEM, LINK_NAME)
 *
 *  @brief Returns a pointer to the previous element in the queue, as linked to
 *         by the link specified with LINK_NAME.
 *
 *  If Q_ELEM is not in a queue or is the first element in the queue,
 *  Q_GET_NEXT should return NULL.
 *
 *  @param Q_ELEM Pointer to the queue element after the desired element
 *  @param LINK_NAME Name of the link organizing the queue
 *
 *  @return The element before Q_ELEM, or NULL if there is no next element
 **/
#define Q_GET_PREV(Q_ELEM, LINK_NAME) \
(\
	/* Q_ELEM non-NULL check */\
	((Q_ELEM) != NULL) ?\
		((void) (Q_ELEM), ((Q_ELEM)->LINK_NAME).prev) :\
		(panic("Variable queue element pointer cannot be NULL"), NULL)\
)

/** @def Q_INSERT_AFTER(Q_HEAD, Q_INQ, Q_TOINSERT, LINK_NAME)
 *
 *  @brief Inserts the queue element Q_TOINSERT after the element Q_INQ
 *         in the queue.
 *
 *  Inserts an element into a queue after a given element. If the given
 *  element is the last element, Q_HEAD should be updated appropriately
 *  (so that Q_TOINSERT becomes the tail element)
 *
 *  @param Q_HEAD head of the queue into which Q_TOINSERT will be inserted
 *  @param Q_INQ  Element already in the queue
 *  @param Q_TOINSERT Element to insert into queue
 *  @param LINK_NAME  Name of link field used to organize the queue
 **/

#define Q_INSERT_AFTER(Q_HEAD,Q_INQ,Q_TOINSERT,LINK_NAME) do\
{\
	/* Pointer argument checks */\
	affirm_msg((Q_HEAD) != NULL,\
	           "Variable queue head pointer cannot be NULL!");\
	affirm_msg((Q_INQ) != NULL,\
	           "Variable queue element pointer in queue cannot be NULL!")\
	affirm_msg((Q_TOINSERT) != NULL,\
	           "Variable queue element pointer to insert cannot be NULL!")\
\
	((Q_TOINSERT)->LINK_NAME).prev = Q_INQ;\
	((Q_TOINSERT)->LINK_NAME).next = ((Q_INQ)->LINK_NAME).next;\
	((Q_INQ)->LINK_NAME).next = Q_TOINSERT;\
\
	/* Insert at tail of queue */\
	if ((Q_INQ) == (Q_HEAD)->tail) {\
		(Q_HEAD)->tail = Q_TOINSERT;\
\
	/* Not at tail of queue, update child's prev */\
	} else {\
		affirm_msg((Q_TOINSERT)->(LINK_NAME).next != NULL,\
				   "Variable queue element pointer's next cannot be NULL!")\
		((((Q_TOINSERT)->LINK_NAME).next)->LINK_NAME).prev = Q_TOINSERT;\
	}\
} while(0)


/** @def Q_INSERT_BEFORE(Q_HEAD, Q_INQ, Q_TOINSERT, LINK_NAME)
 *
 *  @brief Inserts the queue element Q_TOINSERT before the element Q_INQ
 *         in the queue.
 *
 *  Inserts an element into a queue before a given element. If the given
 *  element is the first element, Q_HEAD should be updated appropriately
 *  (so that Q_TOINSERT becomes the front element)
 *
 *  @param Q_HEAD head of the queue into which Q_TOINSERT will be inserted
 *  @param Q_INQ  Element already in the queue
 *  @param Q_TOINSERT Element to insert into queue
 *  @param LINK_NAME  Name of link field used to organize the queue
 **/

#define Q_INSERT_BEFORE(Q_HEAD,Q_INQ,Q_TOINSERT,LINK_NAME) do\
{\
	/* Pointer argument checks */\
	affirm_msg((Q_HEAD) != NULL,\
	           "Variable queue head pointer cannot be NULL!");\
	affirm_msg((Q_INQ) != NULL,\
	           "Variable queue element pointer in queue cannot be NULL!")\
	affirm_msg((Q_TOINSERT) != NULL,\
	           "Variable queue element pointer to insert cannot be NULL!")\
\
	((Q_TOINSERT)->LINK_NAME).next = Q_INQ;\
	((Q_TOINSERT)->LINK_NAME).prev = ((Q_INQ)->LINK_NAME).prev;\
	((Q_INQ)->LINK_NAME).prev = Q_TOINSERT;\
\
	/* Insert at front of queue */\
	if ((Q_INQ) == (Q_HEAD)->front) {\
		(Q_HEAD)->front = Q_TOINSERT;\
\
	/* Not at front of queue, update ancestor's next */\
	} else {\
		affirm_msg((Q_TOINSERT)->(LINK_NAME).prev != NULL,\
				   "Variable queue element pointer's prev cannot be NULL!")\
		((((Q_TOINSERT)->LINK_NAME).prev)->LINK_NAME).next = Q_TOINSERT;\
	}\
} while(0)

/** @def Q_REMOVE(Q_HEAD,Q_ELEM,LINK_NAME)
 *
 *  @brief Detaches the element Q_ELEM from the queue organized by LINK_NAME,
 *         and returns a pointer to the element.
 *
 *  If Q_HEAD does not use the link named LINK_NAME to organize its elements or
 *  if Q_ELEM is not a member of Q_HEAD's queue, the behavior of this macro
 *  is undefined.
 *
 *  @param Q_HEAD Pointer to the head of the queue containing Q_ELEM. If
 *         Q_REMOVE removes the first, last, or only element in the queue,
 *         Q_HEAD should be updated appropriately.
 *  @param Q_ELEM Pointer to the element to remove from the queue headed by
 *         Q_HEAD.
 *  @param LINK_NAME The name of the link used to organize Q_HEAD's queue
 *
 *  @return Void (if you would like to return a value, you may change this
 *                specification)
 **/
#define Q_REMOVE(Q_HEAD,Q_ELEM,LINK_NAME) do\
{\
	/* Pointer argument checks */\
	affirm_msg((Q_HEAD) != NULL,\
	           "Variable queue head pointer cannot be NULL!");\
	affirm_msg((Q_ELEM) != NULL,\
	           "Variable queue element pointer cannot be NULL!");\
\
	/* If Q_ELEM is only element */\
	if (((Q_ELEM) == (Q_HEAD)->front) && ((Q_ELEM) == (Q_HEAD)->tail)) {\
		(Q_HEAD)->front = 0;\
		(Q_HEAD)->tail = 0;\
\
	/* If Q_ELEM is at the front */\
	} else if ((Q_ELEM) == (Q_HEAD)->front) {\
		(Q_HEAD)->front = ((Q_ELEM)->LINK_NAME).next;\
		affirm_msg((Q_HEAD)->front != NULL,\
	               "Variable queue front pointer cannot be NULL!");\
		(((Q_HEAD)->front)->LINK_NAME).prev = 0;\
\
	/* If Q_ELEM is at the tail */\
	} else if ((Q_ELEM) == (Q_HEAD)->tail) {\
		(Q_HEAD)->tail = ((Q_ELEM)->LINK_NAME).prev;\
		affirm_msg((Q_HEAD)->tail != NULL,\
	               "Variable queue tail pointer cannot be NULL!");\
		(((Q_HEAD)->tail)->LINK_NAME).next = 0;\
\
	/* Q_ELEM is somewhere in the middle */\
	} else {\
		affirm_msg((((Q_ELEM)->LINK_NAME).next) != NULL,\
		           "Variable queue element pointer next cannot be NULL");\
		((((Q_ELEM)->LINK_NAME).next)->LINK_NAME).prev = \
			(((Q_ELEM)->LINK_NAME).prev);\
\
		affirm_msg((((Q_ELEM)->LINK_NAME).prev) != NULL,\
		           "Variable queue element pointer prev cannot be NULL");\
		((((Q_ELEM)->LINK_NAME).prev)->LINK_NAME).next = \
			(((Q_ELEM)->LINK_NAME).next);\
	}\
} while(0)

#endif /* _VARIABLE_QUEUE_H_ */
