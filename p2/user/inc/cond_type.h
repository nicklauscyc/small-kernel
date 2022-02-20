/** @file cond_type.h
 *  @brief This file defines the type for condition variables.
 */

#ifndef _COND_TYPE_H
#define _COND_TYPE_H

#include <mutex.h> /* mutex_t */
#include <variable_queue.h>

/** @brief struct for each node in a condition variable's queue
 *
 *  On full macros expansion, the struct will look like this:
 *
 *  typedef struct cvar_node {
 *  	struct {
 *      	struct cvar_node *prev;
 *      	struct cvar_node *next;
 *  	} link;
 *  	mutex_t *mp;
 *  	int tid;
 *  	int descheduled;
 *  } cvar_node_t;
 */
typedef struct cvar_node {
	Q_NEW_LINK(cvar_node) link; /**< struct for pointers to other cvar_node_t */
	mutex_t *mp; /**< pointer to mutex locked by thread on call to con_wait() */
	int tid; /**< thread ID of this blocked thread waiting via con_wait() */
	int descheduled; /**< boolean to "atomically" implement thread deschedule */
} cvar_node_t;

/** @brief queue head for a condition variable's queue
 *
 *  On full macros expansion, the struct will look like this:
 *
 *  typedef struct {
 *  	struct cvar_node *front;
 *  	struct cvar_node *tail;
 *  } cvar_queue_t;
 */
Q_NEW_HEAD(cvar_queue_t, cvar_node);

/** @brief struct for the condition variable
 */
typedef struct cond {
	mutex_t mux; /**< mutex for the condition variable */
	cvar_queue_t qhead; /**< queue head for the condition variable */
	int initialized; /**< 1 if condition variable initialized, 0 otherwise */
} cond_t;



#endif /* _COND_TYPE_H */
