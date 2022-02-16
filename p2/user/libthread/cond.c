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

#include <mutex.h> /* mutex_t */
#include <cond_type.h> /* cond_t */
#include <malloc.h> /* malloc() */
#include <variable_queue.h> /* queue_t */
#include <assert.h> /* assert() */
#include <thr_internals.h>


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
	return 0;
}
//void cond_destroy( cond_t *cv );
void
cond_wait( cond_t *cv, mutex_t *mp )
{
	/* Lock cv mutex */
	mutex_lock(cv->mp);

	//TODO get thread info struct

	/* Add to cv queue */
	//Q_INSERT_TAIL(cv->qp,
	//
	}
//void cond_signal( cond_t *cv );
//void cond_broadcast( cond_t *cv );
//
//#ifndef MUTEX_H
//#define MUTEX_H
//
//#include <mutex_type.h>
//
//int mutex_init( mutex_t *mp );
//void mutex_destroy( mutex_t *mp );
//void mutex_lock( mutex_t *mp );
//void mutex_unlock( mutex_t *mp );
//
//#
