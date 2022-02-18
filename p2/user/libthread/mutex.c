/** @file mutex.c
 *  @brief A mutex object
 *
 *  The mutex is implemented via a ticketing system.
 *
 *  Taking a ticket:
 *  A given thread does an atomic add to m->ticket
 *  and the results is their ticket.
 *
 *  Taking the lock:
 *  If serving == my_ticket, finish taking lock.
 *  Otherwise yield (optimization: deschedule and have last person make_runnable on self).
 *  OPTIM: Can use ticket to serialize threads -> Somehow use this to request make_run from previous thread.
 *
 *  Releasing lock:
 *  Update serving = my_ticket + 1 (optim: make_runnable thread with tid == buffer[serving])
 *
 *  Destroying lock:
 *  Check there are no descheduled waiting threads, if there are fail (if now_serving != ticket).
 *  Otherwise, free and return;
 *
 *  */

#include <mutex_type.h>
#include <mutex.h>
#include <thr_internals.h> /* add_one_atomic */
#include <assert.h>     /* affirm_msg() */
#include <syscall.h>    /* yield() */

int
mutex_init( mutex_t *mp )
{
    if (!mp)
        return -1;
    mp->initialized = 1;
    mp->serving = 0;
    mp->next_ticket = 0;
    mp->owner_tid = -1;
    return 0;
}

void
mutex_destroy( mutex_t *mp )
{
    /* No op if mutex not initialized. */
    if (!mp || !mp->initialized)
        return;
    /* However, crash if threads have or are waiting for lock. */
    affirm_msg(mp->serving == mp->next_ticket,
            "Tried to destroy mutex in use by other threads");
    mp->initialized = 0;
}

/* TODO: Make this re-entrant, ie no-op if calling thread already owns mutex */
void
mutex_lock( mutex_t *mp )
{
    /* Exit if impossible to lock mutex, as just returning would give
     * thread the false impression that lock was acquired. */
    affirm_msg(mp && mp->initialized,
            "Tried to acquire invalid or uninitialized lock");

    /* Get ticket. add_one_atomic returns after addition, so subtract 1 */
    uint32_t my_ticket = add_one_atomic(&mp->next_ticket);
    while (my_ticket != mp->serving) {
        yield(mp->owner_tid); /* Don't busy wait and prioritize lock owner */
	}

    mp->owner_tid = gettid();
}

void
mutex_unlock( mutex_t *mp )
{
    /* Ensure lock is valid, locked and owned by this thread*/
    affirm_msg(mp && mp->initialized,
            "Tried to unlock invalid or uninitialized lock");
    affirm_msg(mp->owner_tid == gettid(),
            "Tried to unlock lock owned by different thread");
    affirm_msg(mp->serving < mp->next_ticket,
            "Tried to unlock mutex that was not locked");

    mp->serving++;
    mp->owner_tid = -1;
}
