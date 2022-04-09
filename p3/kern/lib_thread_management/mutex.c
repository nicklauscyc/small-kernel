/** @file mutex.c
 *  @brief A mutex object
 *
 *  Mutex uses atomically sections for synchronization
 *
 *  @author Andre Nascimento (anascime)
 *  */

#include "mutex.h"
#include <assert.h>     /* affirm_msg() */
#include <scheduler.h>  /* queue_t, make_thread_runnable, run_next_tcb */
#include <asm.h>        /* enable/disable_interrupts() */
#include <logger.h>     /* log */
#include <task_manager_internal.h> /* Q MACRO for tcb */

// TODO: REMOVE ME AND MAGIC_BREAK
#include <simics.h>

static void store_tcb_in_mutex_queue( tcb_t *tcb, void *data );

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
    affirm(0); /* TODO UNIMPLEMENTED */
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
    affirm(mp && mp->initialized);

	/* To simplify the mutex interface, we let a thread run mutex
	 * guarded code even if the scheduler is not initialized. This
	 * is fine because, as soon as we have 2 or more threads, the
	 * scheduler must have been initialized. */
	if (!is_scheduler_init()) {
		mp->owned = 1;
		mp->owner_tid = get_running_tid();
		goto mutex_exit;
	}

	/* If calling thread already owns mutex, panic */
	if (mp->owned && get_running_tid() == mp->owner_tid) {
		panic("Thread trying to reacquire lock %p. Locks are not "
			  "reentrant!", mp);
	}

    /* Atomically check if there is no owner, if so keep going, otherwise
     * add self to queue and let scheduler run next. */
    disable_interrupts(); /* ATOMICALLY { */
    if (!mp->owned) {
        mp->owned = 1;
        mp->owner_tid = get_running_tid();
        enable_interrupts(); /* } */
		log_info("Got lock %p (!mp->owned)", mp);
        goto mutex_exit;
    }
    log_info("Waiting on lock. mp->owned %d, mp->owner_tid %d",
			mp->owned, mp->owner_tid);

	enable_interrupts();

    assert(yield_execution(BLOCKED, -1, store_tcb_in_mutex_queue, mp) == 0);

mutex_exit:
	//assert(mp->owned);
	//assert(mp->owner_tid == get_running_tid());
	if (mp->owner_tid != get_running_tid())
		MAGIC_BREAK;
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
    assert(mp->owned);
	//assert(mp->owner_tid == get_running_tid());
	if (mp->owner_tid != get_running_tid()) {
		MAGIC_BREAK;
	}

	/* If scheduler is not initialized we must have a single
	 * thread, so no one to make runnable. */
	if (!is_scheduler_init()) {
		mp->owned = 0;
		return;
	}

    /* Atomically check if someone is in waiters queue, if so,
     * add them to run queue. */
    disable_interrupts();
    tcb_t *to_run;
    if ((to_run = Q_GET_FRONT(&mp->waiters_queue))) {
        Q_REMOVE(&mp->waiters_queue, to_run, scheduler_queue);
        mp->owner_tid = to_run->tid;
		enable_interrupts();
        make_thread_runnable(to_run->tid);
		log_info("Giving lock %p to %d", mp, to_run->tid);
    } else {
        mp->owned = 0;
    }

    enable_interrupts();
}


static void
store_tcb_in_mutex_queue( tcb_t *tcb, void *data )
{
	affirm(tcb && data && tcb->status == BLOCKED);
	/* Since thread not running, might as well use the scheduler queue link! */
	mutex_t *mp = (mutex_t *)data;
	Q_INIT_ELEM(tcb, scheduler_queue);
	Q_INSERT_TAIL(&mp->waiters_queue, tcb, scheduler_queue);
}
