/** @file sem.c
 *  @brief Implements semaphores
 *  @author Nicklaus Choo (nchoo)
 */

#include <sem_type.h>
#include <mutex.h>
#include <cond.h>
#include <assert.h> /* affirm_msg() */
#include <thr_internals.h> /* tprintf() */



int
sem_init( sem_t *sem, int count )
{
	/* sem cannot be NULL */
	if (!sem) return -1;

	/* TODO check this */
	/* a semaphore initialized to count < 0 results in an unusable semaphore */
	// Apparentyl u can set count to 0
	if (count < 0) return -1;

	if (mutex_init(&(sem->mux)) < 0) return -1;
	if (cond_init(&(sem->cv)) < 0) return -1;

	/* Exclusive access to initialize the count */
	mutex_lock(&(sem->mux));
	sem->total = count;
	sem->count = count;
	sem->initialized = 1;
	sem->muxp = &(sem->mux);
	sem->cvp = &(sem->cv);
	mutex_unlock(&(sem->mux));

	return 0;
}


/* TODO what if sem points to garbage values */
void
sem_wait( sem_t *sem )
{
	/* sem must not be NULL */
	affirm_msg(sem, "argument sem_t *sem must be non-NULL");

	/* Illegal for application to use a semaphore before initialized */
	affirm_msg(sem->initialized, "argument sem_t *sem must be initialized!");

	/* Acquire exclusive access */
	mutex_lock(sem->muxp);

	while (sem->count <= 0) {
		affirm_msg(sem->count == 0, "sem->count cannot go below 0");
		cond_wait(sem->cvp, sem->muxp);
	}
    affirm_msg(sem->count > 0, "sem->count cannot must be nonzero");
	--(sem->count);

	/* Release exclusive accesss */
	mutex_unlock(sem->muxp);

	return;
}

void
sem_destroy( sem_t *sem )
{
	/* Can only destroy initialized semaphores */
	affirm_msg(sem->initialized, "argument sem_t *sem must be initialized!");

	/* Destroy */
    mutex_lock(sem->muxp);

	/* How to know if one is waiting? > 0? */
	affirm_msg(sem->count >= 0, "cannot destroy when threads waiting");
	sem->initialized = 0;
	mutex_unlock(sem->muxp);

	return;
}

/* TODO What if signal will cause count > total? */
void
sem_signal( sem_t *sem)
{
	affirm_msg(sem->initialized, "argument sem_t *sem must be initialized!");

	/* Returning something to semaphore */
	mutex_lock(sem->muxp);

	++(sem->count);
	/* cond signal if some thread is waiting */
	cond_signal(sem->cvp);
	mutex_unlock(sem->muxp);

	return;
}





