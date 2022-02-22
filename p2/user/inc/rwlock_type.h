/** @file rwlock_type.h
 *  @brief This file defines the type for reader/writer locks.
 */

#ifndef _RWLOCK_TYPE_H
#define _RWLOCK_TYPE_H

#include <mutex_type.h>
#include <cond_type.h>

typedef enum { READING, WRITING, NONE } state_t;

typedef struct thr_node {
    int tid;
    struct thr_node *next;
} thr_node_t;

typedef struct rwlock {
    state_t state;      /** Current state of the lock */
    cond_t readers;     /** Queue of threads waiting to read */
    cond_t writers;     /** Queue of threads waiting to write */
    int num_active;     /** List of active lock holders */
    int initialized;    /** Whether this lock has been initialized */
    mutex_t state_mux;  /** Mutex for modifying rwlock state */
} rwlock_t;

#endif /* _RWLOCK_TYPE_H */
