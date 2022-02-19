/** @file cond_type.h
 *  @brief This file defines the type for condition variables.
 */

#ifndef _COND_TYPE_H
#define _COND_TYPE_H

#include <mutex.h> /* mutex_t */
#include <variable_queue.h>

/* Declare type of cvar_node_t */
// TODO not sure where this should go
/** @brief struct for each node in cv->queue
 *
 *  typedef struct cvar_node {
 *  	   struct {
 *  	       struct cvar_node *prev;
 *  	       struct cvar_node *next;
 *  	   }
 *  	   thr_status_t *thr_info;
 *  } cvar_node_t;
 */
typedef struct cvar_node {
	Q_NEW_LINK(cvar_node) link;
	int tid;
	mutex_t *mp;
	int descheduled;
} cvar_node_t;

/* Declare type of cvar_queue_t */
/*
typedef struct {
    struct cvar_node *front;
    struct cvar_node *tail;
} cvar_queue_t;
 */
Q_NEW_HEAD(cvar_queue_t, cvar_node);

/* Condition variable */
typedef struct cond {
	mutex_t *mp;
	cvar_queue_t *qp;
	int init;
} cond_t;



#endif /* _COND_TYPE_H */
