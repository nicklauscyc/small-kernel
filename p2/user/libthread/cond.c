/** @file cond.c
 *
 * 	@brief Implements conditional variables
 *
 * 	Conditional variables are implemented via a queue of waiting threads. For
 * 	each cond_t *cv we have a queue for it. When a thread calls cond_wait()
 * 	after acquiring a mutex lock for some mutex protected state, the thread
 * 	first acquires the lock for cv->mp and is added to cv->queue and the
 * 	struct containing info for that thread is set to indicate the the
 * 	waiting thread is descheduled. cv->mp is unlocked, then the mutex lock for
 * 	the mutex protected state is unlocked. TODO does the order of unlocking
 * 	matter?
 *
 *  @author Nicklaus Choo (nchoo)
 */

#include <simics.h> /* lprintf() */
#include <mutex.h> /* mutex_t */
#include <cond_type.h> /* cond_t */
#include <malloc.h> /* malloc() */
#include <variable_queue.h> /* queue_t */
#include <assert.h> /* assert() */
#include <thr_internals.h> /* thr_status_t */
#include <syscall.h> /* gettid() */
#include <string.h> /* memset() */


/** @brief Initializes a condition variable
 *
 * 	We define a condition variable to be in used when its queue is not empty.
 *
 *  @param cv Pointer to condition variable to be initialized
 */
int
cond_init( cond_t *cv )
{
	/* It is illegal to initialize when already initialized and in use */
	if (cv->initialized && Q_GET_FRONT(&(cv->qhead))) {
		return -1;
	}
	/* Initialize mutex, queue head, and indicate so */
	mutex_init(&(cv->mux));
	Q_INIT_HEAD(&(cv->qhead));
	assert(!Q_GET_FRONT(&(cv->qhead)));
	assert(!Q_GET_TAIL(&(cv->qhead)));
	cv->initialized = 1;
	return 0;
}

/** @brief deallocates memory of cv
 *
 *  Affirm that the queue is empty before destroying so that clients
 *  will know that they have committed an illegal operation, and not
 *  mistakenly think that cv was destroyed (if a no-op was done instead
 *  when cv->queue is not empty.
 *
 *  @param cv Pointer to condition variable to deallocate memory
 */
void
cond_destroy( cond_t *cv )
{
	/* Cannot destroy if queue of blocked threads not empty */
	affirm_msg(!(Q_GET_FRONT(cv->qp)), "Illegal: attempted to destroy "
		"condition variable with blocked threads");

	/* Lock this cv */
	mutex_lock(cv->mp);

 	/* Free queue head */
	free(cv->qp);

	/* Mark as unitialized so no one else may use this cv */
	cv->init = 0;

	/* Release lock and deactivate mutex */
	mutex_unlock(cv->mp);
	mutex_destroy(cv->mp);
	free(cv->mp); //TODO is it ok to free this guy?
}

void
cond_wait( cond_t *cv, mutex_t *mp )
{
	/* If cv has been de-initialized, release lock and do nothing */
	affirm_msg(cv != NULL && cv->init,
            "Trying to wait on uninitialized cond variable.");

    // TODO: cond_destroy could happen here (Illegal, however)

	/* Lock cv mutex */
	mutex_lock(cv->mp);

	/* Allocate memory for linked list element */
	cvar_node_t *cn = malloc(sizeof(cvar_node_t));
	//TODO what if malloc fails?
	assert(cn);
	memset(cn, 0, sizeof(cvar_node_t));

	/* Initialize the node in queue */
	cn->mp = mp;
	cn->tid = gettid();

	/* Mark as descheduled so other threads know this thread not runnable */
	cn->descheduled = 1;
	tprintf("marked as descheduled");

	/* Add to cv queue tail */
	Q_INSERT_TAIL(cv->qp, cn, link);

	/* Give up cv mutex */
	mutex_unlock(cv->mp);

	/* Give up mutex */
	mutex_unlock(mp);

	/* Finally deschedule this thread */
	int runnable = 0;
	int res = deschedule(&runnable);

	/* res should be 0 on successful return */
	assert(!res);

	/* On wake up, reacquire mutex */
	mutex_lock(mp);

	return;
}

/** @brief Helper function which assumes cv->mp is locked by this
 *         thread.
 *
 *  This solves the problem of not awakening threads which may have invoked
 *  cond_wait() after the call to con_broadcast().
 *
 *  Requires that cv->mp be acquired before calling _cond_signal
 *
 *  @param cv Pointer to condition variable cv
 *  @param from_broadcast Boolean set to 1 if called from within
 *  	   con_broadcast(), 0 otherwise.
 */
void
_cond_signal( cond_t *cv )
{
	/* Get front most descheduled thread if queue non_empty */
	cvar_node_t *front = Q_GET_FRONT(cv->qp);

	if (front) {
		Q_REMOVE(cv->qp, front, link);

		/* Update that this is no longer descheduled */
		affirm(front->descheduled);
		front->descheduled = 0;

		/* get tid and make runnable */
		int tid = front->tid;
        int res;

        /* Make runnable will only fail if the thread has not been descheduled yet.
         * However since we know front->descheduled is 1, then that thread is set
         * to be deschedule soon - where soon means in a few instructions. */
		while ((res = make_runnable(tid)) < 0) {
            yield(tid);
        }
		/* free front */
		free(front);
	}
	return;
}

/** @brief Wakes up the first descheduled thread on the queue
 *
 *  @param cv Pointer to condition variable
 */
void
cond_signal( cond_t *cv )
{
	assert(cv);
	mutex_lock(cv->mp);
	_cond_signal(cv);
	mutex_unlock(cv->mp);
}

/** @brief Locks the cv and wakes up all sleeping threads in queue
 *
 *  @param cv Pointer to condition variable
 */
void
cond_broadcast( cond_t *cv )
{
	assert(cv);
	/* Lock cv->mp */
	mutex_lock(cv->mp);

	/* Wake up all threads */
	while(Q_GET_FRONT(cv->qp)) {
		_cond_signal(cv);
	}
	/* Unlock cv->mp */
	mutex_unlock(cv->mp);

	return;
}
