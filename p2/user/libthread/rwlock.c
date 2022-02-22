/** @file rwlock.c
 *  @brief A read/write lock
 *
 *  - Talk about design (coalescing readers)
 *  */

#include <stdlib.h>
#include <mutex.h>
#include <cond.h>
#include <rwlock_type.h> /* rwlock_t */

static void wait_for_turn( rwlock_t *rwlock, state_t state );
static void start_reader_phase( rwlock_t *rwlock );
static void start_writer_phase( rwlock_t *rwlock );

/** @brief Initialize read/write lock.
 *  @param rwlock Memory location in which to initialize lock
 *
 *  @return 0 if successful, negative value otherwise */
int
rwlock_init( rwlock_t *rwlock )
{
    affirm(rwlock);

    rwlock->state = NONE;
    rwlock->initialized = 1;
    rwlock->num_active = 0;
    rwlock->num_waiting_writers = 0;
    rwlock->num_waiting_readers = 0;

    if (cond_init(&rwlock->readers) != 0) return -1;
    if (cond_init(&rwlock->writers) != 0) goto cleanup_A;
    if (mutex_init(&rwlock->state_mux) != 0) goto cleanup_B;

	return 0;

cleanup_B:
    cond_destroy(&rwlock->writers);
cleanup_A:
    cond_destroy(&rwlock->readers);

    return -1;
}

/** @brief Lock read/write lock in read or write mode.
 *  @param rwlock Memory location in which to lock
 *  @param type Type of access to acquire, eithre read or read/write exclusive
 *
 *  @return Void. */
void
rwlock_lock( rwlock_t *rwlock, int type )
{
    affirm(rwlock);
    affirm(type == RWLOCK_READ || type == RWLOCK_WRITE);

    mutex_lock(&rwlock->state_mux);

    affirm(rwlock->initialized);

    state_t requested_state = type == RWLOCK_READ ? READER : WRITER;

    if (rwlock->state == NONE) {
        /* First come, first serve */
        assert(rwlock->num_active == 0);
        rwlock->num_active++;
        rwlock->state = requested_state;

    } else if (rwlock->state == READER) {
        /* If no writers are waiting, just give out reader locks. */
        if (requested_state == READER && rwlock->num_waiting_writers == 0) {
            rwlock->num_active++;
        } else {
            wait_for_turn(rwlock, requested_state);
        }

    } else if (rwlock->state == WRITER) {
        wait_for_turn(rwlock, requested_state);

    } else {
        /* NOTREACHED */
        assert(0);
    }

    mutex_unlock(&rwlock->state_mux);
    return;
}

/** @brief Unlock read/write lock.
 *  @param rwlock Memory location in which to unlock
 *
 *  @return Void. */
void rwlock_unlock( rwlock_t *rwlock )
{
    affirm(rwlock);

    mutex_lock(&rwlock->state_mux);

    // TODO: How can we check that the calling thread currently has
    // access to the critical section?
    affirm(rwlock->initialized);

    if (rwlock->state == READER) {
        affirm(rwlock->num_active > 0);
        rwlock->num_active--;

        /* Last reader out closes the door */
        if (rwlock->num_active == 0)
            start_writer_phase(rwlock);

    } else if (rwlock->state == WRITER) {
        affirm(rwlock->num_active == 1);
        rwlock->num_active = 0;

        start_reader_phase();

    } else {
        /* NOTREACHED */
        assert(0);
    }

    mutex_unlock(&rwlock->state_mux);
	return;
}

/** @brief Destroy read/write lock.
 *  @param rwlock Memory location in which to destroy lock
 *
 *  @return Void. */
void rwlock_destroy( rwlock_t *rwlock )
{
    affirm(rwlock);

    mutex_lock(&rwlock->state_mux);

    /* Ensure no one has or is waiting on rwlock */
    affirm (rwlock->initialized && rwlock->num_active == 0
            && rwlock->num_waiting_readers == 0
            && rwlock->num_waiting_writers == 0);

    /* Cleanup */
    cond_destroy(&rwlock->readers);
    cond_destroy(&rwlock->writers);

    /* Ensure no one locks rwlock->state_mux between
     * us dropping and destroying it. */
    rwlock->initialized = 0;
    mutex_unlock(&rwlock->state_mux);

    mutex_destroy(&rwlock->state_mux);

	return;
}

/** @brief Downgrade read/write lock. Should only be called if this thread
 *         has exclusive read/write access and wishes to forego write
 *         privileges.
 *
 *  @param rwlock Memory location in which to destroy lock
 *
 *  @return Void. */
void rwlock_downgrade( rwlock_t *rwlock )
{
    affirm(rwlock);

    mutex_lock(&rwlock->state_mux);

    // TODO: How can we check that this is the same thread
    // as the one who currently has write access?
    affirm(rwlock->initialized && rwlock->num_active == 1
            && rwlock->state == WRITER);

    /* Since this is a downgrade, the caller becomes a reader
     * and all readers are invited to join.  */
    rwlock->num_active = 1;
    rwlock->state = READER;
    cond_broadcast(&rwlock->readers);

    mutex_unlock(&rwlock->state_mux);

	return;
}

/* ------------------------ Helper functions ------------------------ */

/** @brief Tries to start the writer phase. If there are no writers,
 *         tries to start reader phase. If no readers either, goes to NONE.
 *         Assumes it has rwlock->state_mux and will not unlock it.  */
static void start_writer_phase( rwlock_t *rwlock )
{
    /* Let a single writer in */
    if (rwlock->num_waiting_writers > 0) {
        rwlock->state = WRITER;
        cond_signal(&rwlock->writers);
        return;
    }

    /* If no writer waiting, let all readers in */
    if (rwlock->num_waiting_readers > 0) {
        rwlock->state = READER;
        cond_broadcast(&rwlock->readers);
        return;
    }

    rwlock->state = NONE;
    return;
}

/** @brief Tries to start the reader phase. If there are no readers,
 *         tries to start writer phase. If no writers either, goes to NONE.
 *         Assumes it has rwlock->state_mux and does not unlock it. */
static void start_reader_phase( rwlock_t *rwlock )
{
    /* Let all waiting readers in */
    if (rwlock->num_waiting_readers > 0) {
        rwlock->state = READER;
        cond_broadcast(&rwlock->readers);
        return;
    }

    /* However, if no one's waiting to read let writers have a go */
    if (rwlock->num_waiting_writers > 0) {
        rwlock->state = WRITER;
        cond_signal(&rwlock->writers);
        return;
    }

    rwlock->state = NONE;
    return;
}

/** @brief Wait in writers queue.
 *         Assumes rwlock->state_mux is locked, and returns with it locked. */
static void wait_writer( rwlock_t *rwlock )
{
    rwlock->num_waiting_writers++;

    /* Wait until we have exclusive access */
    while (rwlock->num_active > 0)
        cond_wait(&rwlock->writers, &rwlock->state_mux);

    rwlock->num_waiting_writers--;
    rwlock->num_active++;
}

/** @brief Wait in readers queue.
 *         Assumes rwlock->state_mux is locked, and returns with it locked. */
static void wait_reader( rwlock_t *rwlock )
{
    rwlock->num_waiting_readers++;

    /* Wait until next READER state. This can be triggered early
     * if readers is signaled before a writer has had a chance to
     * lock. Since we expect this to not occurr, this check suffices. */
    while (rwlock->state != READER)
        cond_wait(&rwlock->readers, &rwlock->state_mux);

    rwlock->num_waiting_readers--;
    rwlock->num_active++;
}

/** @brief Wait in queue until we're given access.
 *
 *  Assumes rwlock->state_mux is locked, and when this function
 *  returns, it will be locked. */
static void wait_for_turn( rwlock_t *rwlock, state_t state )
{
    if (state == READER)
        wait_reader(rwlock);
    if (state == WRITER)
        wait_writer(rwlock);
}

