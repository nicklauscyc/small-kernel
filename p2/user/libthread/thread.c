/** @file thread.c
 *
 *  @brief This file implements the Thread Management API declared in
 *         thread.h
 *
 *  @author Nicklaus Choo (nchoo)
 *  @author Andre Nascimento (anascime)
 */

#include <malloc.h> /* malloc() */
#include <thr_internals.h> /* thread_fork() */
#include <thread.h> /* all thread library prototypes */
#include <syscall.h> /* gettid() */
#include <assert.h> /* assert() */
#include <mutex.h> /* mutex_t */
#include <string.h> /* memset() */
#include <cond.h> /* con_wait(), con_signal() */

/* thread library functions */
//int thr_init( unsigned int size );
//int thr_create( void *(*func)(void *), void *args );
//int thr_join( int tid, void **statusp );
//void thr_exit( void *status );
//int thr_getid( void );
//int thr_yield( int tid );

#define ALIGN 16
#define NUM_THREADS 100

#define THR_UNINIT -2
/* Private global variable for non initial threads' stack size */
static unsigned int THR_STACK_SIZE = 0;

extern void *global_stack_low; /* In autostack.c */
mutex_t malloc_mutex; /* global mutex for malloc family functions */

/* Global variable to indicate that thr library has been initialized */
// FIXME: Might not need this as programs are well behaved (section 5.3)
int THR_INITIALIZED = 0;

/* TODO make this unbounded.
 * A dictionary would be ideal. */
// #define NUM_THREADS 100
thr_status_t *tid2thr_statusp[NUM_THREADS];
mutex_t thr_status_mux;

/** @brief Gets a pointer to the struct containing thread status via the tid
 *
 *  @param tid Thread id to get status of
 *  @return Pointer to struct containing all thread status
 */
thr_status_t *
get_thr_status( int tid )
{
	assert(tid >= 0);
	/* Ensure exclusive access to array of all thread status */
	mutex_lock(&thr_status_mux);

	/* Get pointer to thread status based on tid */
	thr_status_t *thr_statusp = tid2thr_statusp[tid];

	/* Give up mutex */
	mutex_unlock(&thr_status_mux);

	return thr_statusp;
}


/** @brief Initializes the size all thread stacks will have if the thread
 *         is not the initial thread.
 *
 *  @param size Stack size for thread in bytes
 *  @return 0 if successful, -1 if size <= 0
 */
int
thr_init( unsigned int size )
{
	if (size == 0 || THR_INITIALIZED) return -1;

	/* Initialize the mutex for malloc family functions. */
	affirm_msg(mutex_init(&malloc_mutex) == 0,
            "Failed to initialize mutex used in malloc library");

    /* Initialize mutex for thread status array, which contains all threads
     * in this library. */
	affirm_msg(mutex_init(&thr_status_mux) == 0,
            "Failed to initialize mutex used in thread library");

	THR_STACK_SIZE = size;
    THR_INITIALIZED = 1;

    /* Store current threads info in tid2thr_statusp, as it is the only
     * thread in this task not created through thr_create. */

	/* Allocate memory for root thread thr_status_t */
	thr_status_t *tp = malloc(sizeof(thr_status_t));
	affirm_msg(tp, "Failed to allocate memory for thread status.");
    memset(tp, 0, sizeof(thr_status_t));

    /* Set tp for root thread */
    tp->tid = gettid();
    tid2thr_statusp[tp->tid] = tp;

	return 0;
}

// TODO: Consider refactoring
/** @brief Creates a new thread to run func(arg)
 *
 *  To prevent overlaps of the create thread's stack memory and the stack
 *  memory of the initial thread, we malloc() a char array called thr_stack of
 *  THR_STACK_SIZE bytes. %ebp and %esp is set to thr_stack_high, which points to index
 *  THR_STACK_SIZE-1 of thr_stack, %eip is set to func
 */
int
thr_create( void *(*func)(void *), void *arg )
{
	/* thr_init() was not called prior, return error */
	if (!THR_INITIALIZED) return THR_UNINIT;

	/* Allocate memory for thread stack */
	// TODO magic number bad
	// Round the stack size up to a multiple of ALIGN bytes
	unsigned int ROUND_UP_THR_STACK_SIZE =
		((THR_STACK_SIZE + ALIGN - 1) / ALIGN) * ALIGN;

    /* Allocate child stack */
	char *thr_stack = malloc(ROUND_UP_THR_STACK_SIZE);
	affirm_msg(thr_stack, "Failed to allocate child stack.");

	/* Allocate memory for thr_status_t */
	thr_status_t *child_tp = malloc(sizeof(thr_status_t));
	affirm_msg(child_tp, "Failed to allocate memory for thread status.");
    memset(child_tp, 0, sizeof(thr_status_t));

    /* Set child_tp values */
	cond_t *exit_cvar = malloc(sizeof(exit_cvar));
	affirm_msg(exit_cvar, "Failed to allocate memory for thread cvar.");
    memset(exit_cvar, 0, sizeof(cond_t));
	cond_init(exit_cvar);
	child_tp->exit_cvar = exit_cvar;

	child_tp->thr_stack_low = thr_stack;
	child_tp->thr_stack_high = thr_stack + THR_STACK_SIZE - ALIGN;
	assert(((uint32_t)child_tp->thr_stack_high) % ALIGN == 0);

    /* Get mutex so that child_tp is stored before child exits (and writes to it) */
    mutex_lock(&thr_status_mux);

	/* Run func on a new thread */ //TODO: How about fork_and_run instead?
	int tid = thread_fork(child_tp->thr_stack_high, func, arg);

    /* In parent thread, update child information. */
    child_tp->tid = tid;

    tid2thr_statusp[tid] = child_tp;

    /* Only release lock after setting tid2thr_statusp[tid] so that
     * the child knows where to write its exit status. */
    mutex_unlock(&thr_status_mux);

	return tid;
}

int
thr_join( int tid, void **statusp )
{
    /* TODO: Check thread exists */
    mutex_lock(&thr_status_mux);
    if (tid < 0 || tid > 100 || tid2thr_statusp[tid] == NULL) {
        return -1;
	}
	mutex_unlock(&thr_status_mux);

	/* con_wait for thread with tid to exit */
    mutex_lock(&thr_status_mux);
	while (1) {

		/* If some other thread already cleaned up, do nothing more, return */
		if (tid2thr_statusp[tid] == NULL) {
			mutex_unlock(&thr_status_mux);
			return -1;
		}
		/* If exited and not cleaned up, break and clean up */
		if (tid2thr_statusp[tid]->exited)
			break;

        cond_wait(tid2thr_statusp[tid]->exit_cvar, &thr_status_mux);
    }
    /* Collect exit status if statusp non-NULL */
    assert(tid2thr_statusp[tid]->exited);
    if (statusp)
        *statusp = tid2thr_statusp[tid]->status;

    /* TODO: Free child stack and thread status and cond var */
    /* TODO: What should we do about root threads stack? Just don't free? */
    if (tid2thr_statusp[tid]->thr_stack_low)
        free(tid2thr_statusp[tid]->thr_stack_low);
    free(tid2thr_statusp[tid]);

    mutex_unlock(&thr_status_mux);

    return 0;
}

// TODO: Update with new
void
thr_exit( void *status )
{
	int tid = gettid();

    mutex_lock(&thr_status_mux);

    assert(tid > 0 && tid < 100 && tid2thr_statusp[tid] != NULL);

    thr_status_t *tp = tid2thr_statusp[tid];
    assert(tp->tid == tid);

    /* Store exit status for retrieval in thr_join */
    tp->exited = 1;
    tp->status = status;

    /* Get exit_cvar and broadcast that this thread has exited */
    //cvar_t *exit_cvar = thr_statusp->exit_cvar;

    mutex_unlock(&thr_status_mux);

	/* Tell all waiting joining threads of exit */
    cond_broadcast(tp->exit_cvar);

    /* Exit thread */
    vanish();
}

int
thr_yield( int tid )
{
	return yield(tid);
}
int
thr_getid( void )
{
	return gettid();
}




