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
#include <thread.h> /* thr_getid() */


mutex_t global_cv_mux;

/** @brief checks if cv has been initialized and gets cv mutex
 *
 *  Acquires a global lock and quickly uses it to check if initialized, and
 *  if it is, grabs the cv lock for the cond_ function to prevent race
 *  conditions with other threads.
 *
 *  @param cv Pointer to condition variable to check
 *  @return Void. Does not return on error
 */
void
check_init_and_lock( cond_t *cv )
{
	tprintf("cond_init_and_lock");

	/* check if cv initialized atomically */
	mutex_lock(&global_cv_mux);
	affirm_msg(cv->init, "Trying to use uninitialized cond variable.");
	mutex_lock(&(cv->mux));
	mutex_unlock(&global_cv_mux);
}

/** @brief Initializes condition variables
 *
 *  cond_init() should be called once and only once by any user program
 *  per condition variable unless that condition variable is de-initialized
 *  by cond_destroy(). i.e. illegal for user program to call cond_init()
 *  on the same condition variable more than once in a row.
 *
 *  @param cv Pointer to condition variable to be initialized
 *  @return 0 on success, -1 on error
 */
int
cond_init( cond_t *cv )
{
	tprintf("cond_init");
	MAGIC_BREAK;
	/* Initialize mutex and condition variable */
	if(mutex_init(&(cv->mux)) < 0)
		return -1;
	tprintf("after mutex init");

	/* Initialize queue of waiting threads, should be empty */
	Q_INIT_HEAD(&(cv->qh));
	tprintf("after queue head init");
	if (Q_GET_FRONT(&(cv->qh)) || Q_GET_TAIL(&(cv->qh)))
		return -1;
	tprintf("after last");

	cv->init = 1;
	tprintf("after cv_init");
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
 *  @return Void.
 */
void
cond_destroy( cond_t *cv )
{
	tprintf("cond_destroy");

	/* Lock this cv */
	mutex_lock(&(cv->mux));

	/* Cannot destroy if queue of blocked threads not empty */
	affirm_msg(!(Q_GET_FRONT(&(cv->qh))), "Illegal: attempted to destroy "
		"condition variable with blocked threads");

	/* Mark as unitialized so no one else may use this cv */
	cv->init = 0;

	/* Release lock and deactivate mutex */
	mutex_unlock(&(cv->mux));
	mutex_destroy(&(cv->mux));
}

/** @brief Causes the thread that calls con_wait() to be descheduled until
 *         woken up by con_signal.
 *
 *  Checking that cv is initialized will effectively penalize the illegal
 *  action of calling cond_wait() on a destroyed mutex.
 *
 *  @param cv Pointer to condition variable
 *  @param mp Mutex that thread is holding on to when calling con_wait()
 *  @return Void.
 */
void
cond_wait( cond_t *cv, mutex_t *mp )
{
	tprintf("cond_wait");

	affirm_msg(cv, "cond variable pointer cannot be NULL");
	affirm_msg(mp, "mutex pointer cannot be NULL");
	affirm_msg(mp->initialized, "mutex must be initialized");
	affirm_msg(mp->owner_tid == thr_getid(), "thread must lock the mutex");

	/* Prevent getting cv getting destroyed after checking its initialized */
	check_init_and_lock(cv);

	/* Create linked list element and init values */
	cvar_node_t cn;
	memset(&cn, 0, sizeof(cvar_node_t));
	cn.mp = mp;
	cn.tid = gettid();
	cn.descheduled = 1;

	/* Add to cv queue tail */
	Q_INSERT_TAIL(&(cv->qh), &cn, link);

	/* Give up cv mutex and mutex */
	mutex_unlock(&(cv->mux));
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
 *  Requires that &(cv->mux) be acquired before calling _cond_signal
 *
 *  @param cv Pointer to condition variable cv
 *  @param from_broadcast Boolean set to 1 if called from within
 *  	   con_broadcast(), 0 otherwise.
 *  @return Void.
 */
void
_cond_signal( cond_t *cv )
{
	tprintf("_cond_signal");

	/* Get front most descheduled thread if queue non_empty */
	cvar_node_t *front = Q_GET_FRONT(&(cv->qh));

	if (front) {
		Q_REMOVE(&(cv->qh), front, link);

		/* Update that this is no longer descheduled */
		affirm(front->descheduled);
		front->descheduled = 0;

		/* get tid and make runnable */
		int tid = front->tid;
        int res;

        /* Make runnable will only fail if thread has not been descheduled yet.
         * However since we know front->descheduled is 1, then that thread will
         * be deschedule soon - where soon means in a few instructions.
		 */
		while ((res = make_runnable(tid)) < 0) {
            yield(tid);
        }
	}
	return;
}

/** @brief Wakes up the first descheduled thread on the queue
 *
 *  @param cv Pointer to condition variable
 *  @return Void.
 */
void
cond_signal( cond_t *cv )
{
	tprintf("cond_signal");

	affirm_msg(cv, "cond variable pointer cannot be NULL");

	/* check if cv initialized atomically */
	check_init_and_lock(cv);
	_cond_signal(cv);
	mutex_unlock(&(cv->mux));
}

/** @brief Locks the cv and wakes up all sleeping threads in queue
 *
 *  @param cv Pointer to condition variable
 *  @return Void.
 */
void
cond_broadcast( cond_t *cv )
{
	tprintf("cond_broadcast");

	affirm_msg(cv, "cond variable pointer cannot be NULL");

	/* check if cv initialized atomically */
	check_init_and_lock(cv);

	/* Wake up all threads */
	while(Q_GET_FRONT(&(cv->qh))) {
		_cond_signal(cv);
	}
	/* Unlock &(cv->mux) */
	mutex_unlock(&(cv->mux));

	return;
}
