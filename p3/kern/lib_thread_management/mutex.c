/** @file mutex.c
 *  @brief A mutex object
 *
 *  Mutex uses atomically sections for synchr
 *
 *  @author Andre Nascimento (anascime)
 *  */

#include <mutex.h>
#include <assert.h>     /* affirm_msg() */
#include <scheduler.h>  /* queue_t, add_to_runnable_queue, run_next_tcb */
#include <asm.h>            /* enable/disable_interrupts() */

/* TODO: Create function to give up execution rights as opposed to run_next_tcb*/

/* Thread queues.*/
static queue_t runnable_q;

typedef struct {
    queue_t waiters_queue;
    int initialized;
    int owner_tid;
    int owned;
} mutex_t;

/** @brief Initialize a mutex
 *  @param mp Pointer to memory location where mutex should be initialized
 *
 *  @return 0 on success, negative number on error
 *  */
int
mutex_init( mutex_t *mp )
{
    if (!mp)
        return -1;

    Q_INIT_HEAD(&mp->waiters_queue);
    mp->initialized = 1;
    mp->owned = 0;

    return 0;
}

/** @brief Destroy a mutex
 *  @param mp Pointer to memory location where mutex should be destroyed
 *
 *  @return Void
 *  */
void
mutex_destroy( mutex_t *mp )
{
    // TODO
}

/** @brief Lock mutex. Re-entrant.
 *  @param mp Mutex to lock
 *
 *  @return Void
 *  */
void
mutex_lock( mutex_t *mp )
{
    /* Exit if impossible to lock mutex, as just returning would give
     * thread the false impression that lock was acquired. */
    assert(mp && mp->initialized);

	/* If calling thread already owns mutex, no-op */
	if (get_running_tid() == mp->owner_tid) return;

    /* Atomically check if there is no owner, if so keep going, otherwise
     * add self to queue and let scheduler run next. */
    disable_interrupts(); /* ATOMICALLY { */
    if (!mp->owned) {
        mp->owned = 1;
        mp->owner_tid = get_running_tid();
        enable_interrupts(); /* } */
        return;
    }

    run_next_tcb(mp->waiters_queue, BLOCKED);
}

/** @brief Unlock mutex.
 *  @param mp Mutex to unlock. Has to be previously locked
 *            by calling thread
 *
 *  @return Void
 *  */
void
mutex_unlock( mutex_t *mp )
{
    /* Ensure lock is valid, locked and owned by this thread*/
    assert(mp && mp->initialized);
    assert(mp->owned && mp->owner_tid == gettid());

    /* Atomically check if someone is in waiters queue, if so,
     * add them to run queue. */
    disable_interrupts();
    tcb_t *to_run;
    if ((to_run = Q_GET_FRONT(mp->waiters_queue))) {
        Q_REMOVE(mp->waiters_queue, to_run, thr_queue);
        add_to_runnable_queue(tcb);
    }

    mp->owned = 0;
    enable_interrupts();
}
