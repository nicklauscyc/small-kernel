/** @file cond.c
 *
 * 	@brief Implements conditional variables
 *
 *  @author Nicklaus Choo (nchoo)
 */

#include <mutex.h> /* mutex_t */
#include <cond_type.h> /* cond_t */
#include <malloc.h> /* malloc() */
#include <variable_queue.h> /* queue_t */
#include <assert.h> /* assert() */

/* condition variable functions */
int
cond_init( cond_t *cv )
{

	/* Allocate memory for mutex and initialize mutex */
	mutex_t *mp = malloc(sizeof(mutex_t));
	mutex_init(mp);
	cv->mutex = mutex

	/* Allocate memory for queue and initialize queue for waiting threads */
	queue_t *q = malloc(sizeof(queue_t));
	Q_INIT_HEAD(q);
	assert(!Q_GET_FRONT(q));
	assert(!Q_GET_TAIL(q));
	cv->q = q;

	return 0;
}
//void cond_destroy( cond_t *cv );
//void cond_wait( cond_t *cv, mutex_t *mp );
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
