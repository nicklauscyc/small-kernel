/** @file mutex.c
 *  @brief A mutex object
 *
 *  The mutex is implemented via a ticketing system. On lock
 *  a thread atomically gets a ticket (add_one_atomic) and
 *  waits until it is their turn. To solve the problem of
 *  communication among threads (ie. thread 7 should let
 *  thread 8 know it is done) we use a shared memory location,
 *  namely the now_serving variable.
 *
 *  Threads repeatedly check it, yielding to the current lock
 *  owner to avoid wasting cycles, and eventually acquire the lock.
 *
 *  Unlocking simply consists of update the now_serving variable
 *  with a blind write.
 *
 *  @author Andre Nascimento (anascime)
 *  */

#include <mutex_type.h>
#include <mutex.h>
#include <thr_internals.h> /* add_one_atomic */
#include <assert.h>     /* affirm_msg() */
#include <syscall.h>    /* yield() */

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

    mp->initialized = 1;
    mp->serving = 0;
    mp->next_ticket = 0;
    mp->owner_tid = -1;
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
    /* No op if mutex not initialized. */
    if (!mp || !mp->initialized)
        return;
    /* However, crash if threads have or are waiting for lock. */
    affirm_msg(mp->serving == mp->next_ticket && mp->owner_tid == -1,
            "tid[%d]: Tried to destroy mutex in use by other threads",
			gettid());
    mp->initialized = 0;
}

/** @brief Lock mutex. Re-entrant.
 *  @param mp Mutex to lock
 *
 *  @return Void
 *  */
void
mutex_lock( mutex_t *mp )
{
	/* If calling thread already owns mutex, no-op */
	if (gettid() == mp->owner_tid) return;

    /* Exit if impossible to lock mutex, as just returning would give
     * thread the false impression that lock was acquired. */
    affirm_msg(mp && mp->initialized,
            "tid[%d]: Tried to acquire invalid or uninitialized lock",
			 gettid());

    /* Get ticket. add_one_atomic returns after addition, so subtract 1 */
    uint32_t my_ticket = add_one_atomic(&mp->next_ticket);
    while (my_ticket != mp->serving) {
        yield(mp->owner_tid); /* Don't busy wait and prioritize lock owner */
	}

    mp->owner_tid = gettid();
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
    affirm_msg(mp && mp->initialized,
            "tid[%d]: Tried to unlock invalid or uninitialized lock", gettid());
    affirm_msg(mp->owner_tid == gettid(),
            "tid[%d]: Tried to unlock lock owned by tid[%d] ",
			gettid(), mp->owner_tid);
    affirm_msg(mp->serving < mp->next_ticket,
            "tid[%d]: Tried to unlock mutex that was not locked", gettid());

    mp->serving++;
    mp->owner_tid = -1;
}
