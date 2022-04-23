/** @file rwlock_type.h
 *  @brief This file defines the type for reader/writer locks.
 */

#ifndef _RWLOCK_TYPE_H
#define _RWLOCK_TYPE_H

#include <mutex_type.h>
#include <cond_type.h>

/** @brief Enumerator for possible states of a r/w lock */
typedef enum { READING, WRITING, NONE } state_t;

/** @brief Struct containing all info for the read/write lock
 *
 *  @param state Current state of the lock
 *  @param readers Queue of threads waiting to read
 *  @param writers Queue of threads waiting to write
 *  @param num_active List of active lock holders
 *  @param num_waiting_readers Number of readers waiting in queue
 *  @param num_waiting_writers Number of writers waiting in queue
 *  @param initialized Whether this lock has been initialized
 *  @param state_mux Mutex for modifying rwlock state
 */
typedef struct rwlock {
    state_t state;
    cond_t readers;
    cond_t writers;
    int num_active;
    int num_waiting_readers;
    int num_waiting_writers;
    int initialized;
    mutex_t state_mux;
} rwlock_t;

#endif /* _RWLOCK_TYPE_H */
