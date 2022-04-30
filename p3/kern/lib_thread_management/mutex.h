/** @file mutex.h
 *
 *  Definitions for mutex */

#ifndef MUTEX_H_
#define MUTEX_H_

#include <scheduler.h> /* queue_t */

/** @brief A mutex struct
 *
 *  @param waiters_queue Queue of threads waiting on this mutex
 *  @param initialized	 Whether this mutex has been initialized
 *  @param owner_tid	 Tid of this mutex's owner
 *  @param owned		 Whether this mutex is owned
 *  */
struct mutex {
    queue_t waiters_queue;
    int initialized;
    int owner_tid;
    int owned;
};

typedef struct mutex mutex_t;

int mutex_init( mutex_t *mp );
void mutex_destroy( mutex_t *mp );
void mutex_lock( mutex_t *mp );
void mutex_unlock( mutex_t *mp );
void switch_safe_mutex_unlock( mutex_t *mp );

#endif /* MUTEX_H_ */
