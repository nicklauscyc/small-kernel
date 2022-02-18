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

/* thread library functions */
//int thr_init( unsigned int size );
//int thr_create( void *(*func)(void *), void *args );
//int thr_join( int tid, void **statusp );
//void thr_exit( void *status );
//int thr_getid( void );
//int thr_yield( int tid );

#define THR_UNINIT -2
/* Private global variable for non initial threads' stack size */
static unsigned int THR_STACK_SIZE = 0;

extern void *global_stack_low; /* In autostack.c */
mutex_t mmp; /* global mutex for malloc family functions */

/* Global variable to indicate that thr library has been initialized */
// FIXME: Might not need this as programs are well behaved (section 5.3)
int THR_INITIALIZED = 0;


/** @brief Struct containing all necesary information about a thread.
 *
 *  @param thr_stack_low Lowest valid memory address in thread stack
 *  @param thr_stack_high Highest valid memory address in thread stack
 *  @param tid Thread ID (matching that of gettid() syscall)
 *  @param exit_cvar A cvar which will receive a broadcast on thr_exit
 *  @param exited Whether the thread has exited
 *  @param statusp Status which the thread exited with. Valid iff exited == 1
 */
typedef struct {
	char *thr_stack_low;
	char *thr_stack_high;
	int tid;
    //TODO: cond_t *exit_cvar;
    int exited;
    void *status;
} thr_status_t;

/* TODO make this unbounded.
 * A dictionary would be ideal. */
#define NUM_THREADS 100
static thr_status_t *thr_arr[NUM_THREADS];
mutex_t thr_status_mux;

/** @brief Initializes the size all thread stacks will have if the thread
 *         is not the initial thread.
 *
 *  @param size Stack size for thread in bytes
 *  @return 0 if successful, -1 if size <= 0
 */
int
thr_init( unsigned int size )
{
	/* Initialize the mutex for malloc family functions. After this only
	 * malloc() without underscores should be called */
	affirm_msg(mutex_init(&mmp) == 0,
            "Failed to initialize mutex used in malloc library");

    /* Initialize mutex for thread status array, which contains all threads
     * in this library. */
	affirm_msg(mutex_init(&thr_status_mux) == 0,
            "Failed to initialize mutex used in thread library");

	if (size == 0 || THR_INITIALIZED) return -1;
	THR_STACK_SIZE = size;
    THR_INITIALIZED = 1;
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
	// Round the stack size up to a multiple of 4 bytes
	unsigned int ROUND_UP_THR_STACK_SIZE =
		((THR_STACK_SIZE + 4 - 1) / 4) * 4;

    /* Allocate child stack */
	char *thr_stack = malloc(ROUND_UP_THR_STACK_SIZE);
	affirm_msg(thr_stack, "Failed to allocate child stack.");

	/* Allocate memory for thr_status_t */
	thr_status_t *child_tp = malloc(sizeof(thr_status_t));
	affirm_msg(child_tp, "Failed to allocate memory for thread status.");
    memset(child_tp, 0, sizeof(thr_status_t));

    /* Set child_tp values */
	child_tp->thr_stack_low = thr_stack;
	child_tp->thr_stack_high = thr_stack + THR_STACK_SIZE - 4;
	assert(((uint32_t)child_tp->thr_stack_high) % 4 == 0);

    /* Get mutex so that child_tp is stored before child exits (and writes to it) */
    mutex_lock(&thr_status_mux);

	/* Run func on a new thread */ //TODO: How about fork_and_run instead?
	int tid = thread_fork(child_tp->thr_stack_high, func, arg);

    /* In parent thread, update child information. */
    child_tp->tid = tid;

    thr_arr[tid] = child_tp;

    /* Only release lock after setting thr_arr[tid] so that
     * the child knows where to write its exit status. */
    mutex_unlock(&thr_status_mux);

	return tid;
}

int
thr_join( int tid, void **statusp )
{
    /* TODO: Check thread exists */
    mutex_lock(&thr_status_mux);
    if (tid < 0 || tid > 100 || thr_arr[tid] == NULL)
        return -1;

    /* FIXME: Swap for cvar: Wait until thread exits */
    while(1) {
        mutex_lock(&thr_status_mux);
        if (thr_arr[tid]->exited) break;
        mutex_unlock(&thr_status_mux);
        yield(-1);
    }

    // TODO
    // mutex_lock(&thr_status_mux);
    // while (1) {
    //     cond_wait(thr_arr[tid]->exit_cvar, &thr_status_mux);
    //     if (thr_arr[tid]->exited) break;
    // }

    /* Collect exit status */
    assert(thr_arr[tid]->exited);
    *statusp = thr_arr[tid]->status;

    /* TODO: Free child stack and thread status and cond var */
    free(thr_arr[tid]->thr_stack_low);
    free(thr_arr[tid]);

    mutex_unlock(&thr_status_mux);

    return 0;
}

// TODO: Update with new
void
thr_exit( void *status )
{
	int tid = gettid();

    mutex_lock(&thr_status_mux);

    assert(tid > 0 && tid < 100 && thr_arr[tid] != NULL);

    thr_status_t *thr_status = thr_arr[tid];
    assert(thr_status->tid == tid);

    /* Store exit status for retrieval in thr_join */
    thr_status->exited = 1;
    thr_status->status = status;

    /* Get exit_cvar and broadcast that this thread has exited */
    //cvar_t *exit_cvar = thr_status->exit_cvar;

    mutex_unlock(&thr_status_mux);

    // TODO: cond_broadcast(exit_cvar);

    /* Exit thread */
    vanish();
}

int
thr_getid( void )
{
	return gettid();
}




