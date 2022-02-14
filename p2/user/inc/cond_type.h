/** @file cond_type.h
 *  @brief This file defines the type for condition variables.
 */

#ifndef _COND_TYPE_H
#define _COND_TYPE_H

#include <mutex.h> /* mutex_t */
#include <variable_queue.h>


typedef struct node {
	Q_NEW_LINK(node) link;
	int tid;
} node_t;

Q_NEW_HEAD(queue_t, node);

/* Size of the queue */
#define Q_LEN 100

typedef struct cond {
	mutex_t mutex;
	queue_t queue;
} cond_t;

#endif /* _COND_TYPE_H */
