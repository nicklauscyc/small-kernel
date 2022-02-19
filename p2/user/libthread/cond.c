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
//typedef struct {
//	char *thr_stack_low;
//	char *thr_stack_high;
//	int tid;
//    //TODO: cond_t *exit_cvar;
//    int exited;
//    void *status;
//} thr_status_t;

#include <mutex.h> /* mutex_t */
#include <cond_type.h> /* cond_t */
#include <malloc.h> /* malloc() */
#include <variable_queue.h> /* queue_t */
#include <assert.h> /* assert() */
#include <thr_internals.h> /* thr_status_t */
#include <syscall.h> /* gettid() */
#include <string.h> /* memset() */


/* condition variable functions */
int
cond_init( cond_t *cv )
{
	/* Allocate memory for mutex and initialize mutex */
	mutex_t *mp = malloc(sizeof(mutex_t));
	//TODO malloc -1?
	assert(mp);
	mutex_init(mp);
	cv->mp = mp;

	/* Allocate memory for queue and initialize queue for waiting threads */
	cvar_queue_t *qp = malloc(sizeof(cvar_queue_t));
	assert(qp);
	//TODO malloc -1?
	Q_INIT_HEAD(qp);
	assert(!Q_GET_FRONT(qp));
	assert(!Q_GET_TAIL(qp));
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
	free(cv->mp);

	/* Free outermost cv struct itself */
	free(cv);
}


void
cond_wait( cond_t *cv, mutex_t *mp )
{
	tprintf("my id: %d", gettid());
	/* Lock cv mutex */
	tprintf("locking cv->mp");
	mutex_lock(cv->mp);
	tprintf("locked cv->mp");

	/* If cv has been de-initialized, release lock and do nothing */
	if (!cv->init) {
		tprintf("unlocking cv->mp because !cv->init");
		mutex_unlock(cv->mp);
		tprintf("unlocked cv->mp because !cv->init");

		return;
	}
	/* Get thread status struct */
	int tid = gettid();
	thr_status_t *tstatusp = get_thr_status(tid);

	/* Allocate memory for linked list element */
	cvar_node_t *cn = malloc(sizeof(cvar_node_t));
	assert(cn);
	memset(cn, 0, sizeof(cvar_node_t));

	/* Initialize the node in queue */
	cn->tstatusp = tstatusp;
	cn->mp = mp;
	cn->descheduled = 1;

	/* Add to cv queue tail */
	Q_INSERT_TAIL(cv->qp, cn, link);

	/* Give up cv mutex */
	tprintf("unlocking cv->mp");
	mutex_unlock(cv->mp);
	tprintf("unlocked cv->mp");

	/* Give up mutex */
	tprintf("unlocking mp");
	mutex_unlock(mp);
	tprintf("unlocked mp");

	/* Finally deschedule this thread */
	int runnable = 0;
	int res = deschedule(&runnable);

	/* res should be 0 on successful return */
	assert(!res);

	/* On wake up, reacquire mutex */
	tprintf("woken up, trying to acquire mutex");
	mutex_lock(mp);
	tprintf("woken up, acquired mutex");


	return;
}

/** @brief Helper function to decide whether to lock condition variable
 *         mutex or not.
 *
 *  This solves the problem of not awakening threads which may have invoked
 *  cond_wait() after the call to con_broadcast()
 *
 *  @param cv Pointer to condition variable cv
 *  @param from_broadcast Boolean set to 1 if called from within
 *  	   con_broadcast(), 0 otherwise.
 */
void
_cond_signal( cond_t *cv, const int from_broadcast)
{
	/* Acquire mutex if call was not from broadcast */
	if (!from_broadcast)
		mutex_lock(cv->mp);

	/* Get front most descheduled thread if queue non_empty */
	cvar_node_t *front = Q_GET_FRONT(cv->qp);
	if (front) {
		Q_REMOVE(cv->qp, front, link);

		/* Re-acquire mutex and set this thread to runnable */
		mutex_lock(front->mp);

		/* Update that this is no longer descheduled */
		assert(front->descheduled);
		front->descheduled = 0;

		/* get tid and make runnable */
		int tid = (front->tstatusp)->tid;
		make_runnable(tid);
		tprintf("_con_signal make runnable tid[%d]", tid);
	}

	/* Return lock if needed */
    if (!from_broadcast)
		mutex_unlock(cv->mp);

	return;
}

/** @brief Wakes up the first descheduled thread on the queue
 *
 *  @param cv Pointer to condition variable
 */
void
cond_signal( cond_t *cv )
{
	/* Indicate that we are calling cond_signal not from a broadcast */
	_cond_signal(cv, 0);
}

/** @brief Locks the cv and wakes up all sleeping threads in queue
 *
 *  @param cv Pointer to condition variable
 */
void
cond_broadcast( cond_t *cv )
{
	/* Lock cv->mp */
	mutex_lock(cv->mp);

	/* Wake up all threads */
	while(Q_GET_FRONT(cv->qp)) {
		tprintf("con_broadcast() loop\n");
		_cond_signal(cv, 1);
	}
	/* Unlock cv->mp */
	mutex_unlock(cv->mp);

	return;
}
