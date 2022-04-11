#ifndef MUTEX_H_
#define MUTEX_H_

#include <scheduler.h> /* queue_t */

/* TODO: Move to internal .h file? */
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
