/** @file cond_type.h
 *  @brief This file defines the type for condition variables.
 *
 *  There are some important warnings for users using the condition variables.
 *  The most important is to never lock or unlock or interact directly with
 *  the mutex for condition variables in any way. Please stick to the
 *  interface of condition variables or else risk undefined behavior.
 *
 *  @author Nicklaus Choo (nchoo)
 */

#ifndef _COND_TYPE_H
#define _COND_TYPE_H

#include <mutex.h> /* mutex_t */
#include <variable_queue.h> /* Q_NEW_LINK(), Q_NEW_HEAD() */

/** @brief Struct for each node in condition variable queue
 *
 *  On macro expansion, the entire struct looks like this:
 *
 *  typedef struct cvar_node {
 *  	   struct {
 *  	       struct cvar_node *prev;
 *  	       struct cvar_node *next;
 *  	   } link;
 *  	   int tid;
 *  	   mutex_t *mp;
 *  	   int descheduled;
 *  } cvar_node_t;
 *
 *  @param link Contains pointers to previous and next nodes on the linked list
 *  @param tid Thread ID of thread waiting on this condition variable
 *  @param mp Mutex that thread was holding on to on cond_wait() call
 *  @param descheduled 1 if thread is set to be descheduled, 0 otherwise
 */
typedef struct cvar_node {
	Q_NEW_LINK(cvar_node) link;
	int tid;
	mutex_t *mp;
	int descheduled;
} cvar_node_t;

/** @brief Struct for queue head of the condition variable
 *
 * 	On macro expansion, the entire struct looks like this:
 *
 *  typedef struct {
 * 	    struct cvar_node *front;
 *      struct cvar_node *tail;
 *  } cvar_queue_t;
 *
 *  @param front Front (first element) of queue.
 *  @param tail Tail (last element) of queue
 */
Q_NEW_HEAD(cvar_queue_t, cvar_node);

/** @brief Struct for the condition variable.
 *
 *  WARNING: DO NOT EVER LOCK/UNLOCK/INTERACT WITH THE MUTEX FOR CONDITION
 *  VARIABLES DIRECTLY. THE CONDITION VARIABLE MUTEX SHOULD ONLY BE
 *  USED BY OTHER cond_ FUNCTIONS. DISOBEYING THIS INTERFACE WILL LEAD TO
 *  UNDEFINED BEHAVIOR.
 *
 *  @param mp Mutex that the condition variable uses to guard its queue
 *  @param qp Pointer to head of the queue of waiting threads
 *  @param init 1 if condition variable has been initialized, 0 otherwise
 */
typedef struct cond {
	mutex_t mux;
	cvar_queue_t qh;
	int init;
} cond_t;

#endif /* _COND_TYPE_H */
