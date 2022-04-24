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
 * 	the mutex protected state is unlocked.
 *
 *  To avoid interference from other programs calling make_runnable we use
 *  a variable named should_wakeup to keep track of whether cond_signal was
 *  the one who woke us up.
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


/** @brief Initializes condition variables
 *
 *  @param cv Pointer to condition variable to be initialized
 *  @return 0 on success, -1 on error
 */
int
cond_init( cond_t *cv )
{
	/* Allocate memory for mutex and initialize mutex */
	mutex_t *mp = malloc(sizeof(mutex_t));
	if (!mp)
		return -1;
	mutex_init(mp);
	cv->mp = mp;

	/* Allocate memory for queue and initialize queue for waiting threads */
	cvar_queue_t *qp = malloc(sizeof(cvar_queue_t));
	if (!qp) {
		free(mp);
		return -1;
	}
	/* Q head should be empty */
	Q_INIT_HEAD(qp);
	if (Q_GET_FRONT(qp) || Q_GET_TAIL(qp)) {
		free(mp);
		free(qp);
		return -1;
	}
	cv->qp = qp;
	cv->init = 1;
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
	/* Lock this cv */
	mutex_lock(cv->mp);

	/* Cannot destroy if queue of blocked threads not empty */
	affirm_msg(!(Q_GET_FRONT(cv->qp)), "Illegal: attempted to destroy "
		"condition variable with blocked threads");

 	/* Free queue head */
	free(cv->qp);

	/* Mark as unitialized so no one else may use this cv */
	cv->init = 0;

	/* Release lock and deactivate mutex */
	mutex_unlock(cv->mp);
	mutex_destroy(cv->mp);
	free(cv->mp);
}

/** @brief Causes the thread that calls con_wait() to be descheduled until
 *         woken up by con_signal.
 *
 *  @param cv Pointer to condition variable
 *  @param mp Mutex that thread is holding on to when calling con_wait()
 *  @return Void.
 */
void
cond_wait( cond_t *cv, mutex_t *mp )
{
	affirm_msg(cv, "cond variable pointer cannot be NULL");
	affirm_msg(mp, "mutex pointer cannot be NULL");
	affirm_msg(mp->initialized, "mutex must be initialized");
	affirm_msg(mp->owner_tid == thr_getid(), "thread must lock the mutex");
	affirm_msg(cv->init, "Trying to use uninitialized cond variable.");
	mutex_lock(cv->mp);


	/* Initialize linked list element */
	cvar_node_t cn;
	memset(&cn, 0, sizeof(cvar_node_t));
	cn.mp = mp;
	cn.tid = gettid();
	cn.descheduled = 1;
    cn.should_wakeup = 0;

	/* Add to cv queue tail */
	Q_INSERT_TAIL(cv->qp, &cn, link);


	/* Give up cv mutex */
	mutex_unlock(cv->mp);

	/* Give up mutex */
	mutex_unlock(mp);

	/* Finally deschedule this thread */
	int runnable = 0;

    /* Wait until we are woken up by cond_signal */
    while (!cn.should_wakeup) {
	    int res = deschedule(&runnable);
        /* res should be 0 on successful return */
        assert(!res);
    }


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
 *  @return Void.
 */
void
_cond_signal( cond_t *cv )
{
	affirm_msg(cv, "cond variable pointer cannot be NULL");
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

        /* Make runnable will only fail if thread has not been descheduled yet.
         * However since we know front->descheduled is 1, then that thread will
         * be deschedule soon - where soon means in a few instructions.
		 */
        front->should_wakeup = 1;
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
	affirm_msg(cv, "cond variable pointer cannot be NULL");
	affirm_msg(cv->init, "Trying to use uninitialized cond variable.");
	mutex_lock(cv->mp);

	_cond_signal(cv);
	mutex_unlock(cv->mp);
}

/** @brief Locks the cv and wakes up all sleeping threads in queue
 *
 *  @param cv Pointer to condition variable
 *  @return Void.
 */
void
cond_broadcast( cond_t *cv )
{
	affirm_msg(cv, "cond variable pointer cannot be NULL");
	affirm_msg(cv->init, "Trying to use uninitialized cond variable.");
	mutex_lock(cv->mp);

	/* Wake up all threads */
	while(Q_GET_FRONT(cv->qp)) {
		_cond_signal(cv);
	}
	/* Unlock cv->mp */
	mutex_unlock(cv->mp);

	return;
}
