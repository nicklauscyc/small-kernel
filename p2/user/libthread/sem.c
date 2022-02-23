/** @file sem.c
 *  @brief Implements semaphores
 *
 *  Semaphores are implemented with condition variables and mutexes. First
 *  recall the definition (not implementation) of semaphores from OSC 10:
 *
 *  There are 2 functions:
 *
 *  wait(S) {
 *  	while (S <= 0)
 *  		; // busy wait
 *  	S--;
 *  }
 *
 *  signal(S) {
 *  	S++;
 *  }
 *
 *  And so notice that from the definition we implement busy waiting with
 *  condition variables. To access the count in the semaphore we use
 *  mutexes to ensure no race conditions - provided that the interface of
 *  semaphores is respected and clients do not interact directly with
 *  the condition variables and mutexes inside the semaphore data structure.
 *
 *  @author Nicklaus Choo (nchoo)
 */

#include <sem_type.h>
#include <mutex.h>
#include <cond.h>
#include <assert.h> /* affirm_msg() */
#include <thr_internals.h> /* tprintf() */

/** @brief Checks to see if a semaphore that has been initialized is
 *         well-formed
 *
 *  @param sem Pointer to initialized semaphore to be checked
 */
int
is_sem( sem_t *sem )
{
	/* Cannot be NULL */
	assert(sem);

	/* Must be intialized */
	assert(sem->initialized == 1);

	//assert(sem->mux);
	//
	return 1;
	}


/** @brief Initializes the semaphore. Negative count is possible as that does
 *         not violate the definition of a semaphore.
 *
 *  Requires that the semaphore struct is not-corrupted.
 *
 *  @param sem Pointer to semaphore data structure to be initialized
 *  @param count Count to initialize semaphore to.
 */
int
sem_init( sem_t *sem, int count )
{
	/* sem cannot be NULL */
	if (!sem) return -1;

	//TODO error for initialized and in use
	/* check if initialized and in use */
	//if (sem->initialized) {
	//	if (Q_GET_FRONT(

	if (mutex_init(&(sem->mux)) < 0) return -1;
	if (cond_init(&(sem->cv)) < 0) {
		mutex_destroy(&(sem->mux));
		return -1;
	}
	/* Exclusive access to initialize the count */
	mutex_lock(&(sem->mux));
	sem->count = count;
	sem->initialized = 1;
	mutex_unlock(&(sem->mux));

	return 0;
}


/** @brief Waits for semaphore if count <= 0, unblocks and decrements
 *         count once it's done waiting.
 *
 *  @param sem Pointer to semaphore
 */
/* TODO what if sem points to garbage values */
void
sem_wait( sem_t *sem )
{
	/* sem must not be NULL */
	affirm_msg(sem, "argument sem_t *sem must be non-NULL");

	/* Illegal for application to use a semaphore before initialized */
	affirm_msg(sem->initialized, "argument sem_t *sem must be initialized!");

	/* Acquire exclusive access */
	mutex_lock(&(sem->mux));

	while (sem->count <= 0) {
		affirm_msg(sem->count == 0, "sem->count cannot go below 0");
		cond_wait(&(sem->cv), &(sem->mux));
	}
    affirm_msg(sem->count > 0, "sem->count cannot must be nonzero");
	--(sem->count);

	/* Release exclusive accesss */
	mutex_unlock(&(sem->mux));

	return;
}

/** @brief Destroys the semaphore
 *
 *  @param sem Pointer to semaphore to destroy
 */
void
sem_destroy( sem_t *sem )
{
	/* Can only destroy initialized semaphores */
	affirm_msg(sem->initialized, "argument sem_t *sem must be initialized!");

	/* Destroy */
    mutex_lock(&(sem->mux));

	/* How to know if one is waiting? > 0? */
	affirm_msg(sem->count >= 0, "cannot destroy when threads waiting");
	sem->initialized = 0;
	mutex_unlock(&(sem->mux));

	return;
}

/** @brief Signals that the semaphore has increased resource
 *
 * 	@param sem Pointer to semaphore
 */
void
sem_signal( sem_t *sem)
{
	affirm_msg(sem->initialized, "argument sem_t *sem must be initialized!");

	/* Returning something to semaphore */
	mutex_lock(&(sem->mux));

	++(sem->count);
	/* cond signal if some thread is waiting */
	cond_signal(&(sem->cv));
	mutex_unlock(&(sem->mux));

	return;
}

