/** @file thread.c
 *
 *  @brief This file implements the Thread Management API declared in
 *         thread.h
 *
 *  Threads are 1:1 with kernel threads. Communication across threads
 *  is managed through a tid -> thread_status hashmap. This is used
 *  to enable join to receive exit status from exiting thread, as well
 *  as to clean up its resources.
 *
 *  @author Nicklaus Choo (nchoo)
 *  @author Andre Nascimento (anascime)
 *
 *  @bugs No know bugs.
 */

#include <malloc.h> /* malloc() */
#include <thr_internals.h> /* thread_fork() */
#include <thread.h> /* all thread library prototypes */
#include <syscall.h> /* gettid() */
#include <assert.h> /* assert() */
#include <mutex.h> /* mutex_t */
#include <string.h> /* memset() */
#include <cond.h> /* con_wait(), con_signal() */
#include <simics.h> /* MAGIC_BREAK */
#include <ureg.h> /* ureg_t */
#include <autostack_internals.h> /* child_pf_handler(), Swexn() */

/* Align child thread stack to 4 bytes */
#define ALIGN 4

#define NUM_BUCKETS 1024

#define THR_UNINIT -2
/* Private global variable for non initial threads' stack size */
static unsigned int THR_STACK_SIZE = 0;

extern void *global_stack_low; /* In autostack.c */
mutex_t malloc_mutex; /* global mutex for malloc family functions */

/** @brief Indicates whether thr library has been initialized */
int THR_INITIALIZED = 0;

/** @brief Map to store information from each existing thread */
hashmap_t tid2thr_status;
hashmap_t *tid2thr_statusp = &tid2thr_status;

/** @brief Mutex for thread status info. */
mutex_t thr_status_mux;


/** @brief Initializes the size all thread stacks will have if the thread
 *         is not the initial thread.
 *
 *  @param size Stack size for thread in bytes, must be greater than  0
 *  @return 0 if successful, negative value on failure
 */
int
thr_init( unsigned int size )
{
	if (size == 0 || THR_INITIALIZED)
        return -1;

    /* Round thread stack size up to ALIGN bytes */
	THR_STACK_SIZE = ((size + ALIGN - 1) / ALIGN) * ALIGN;
    THR_INITIALIZED = 1;

	/* Initialize the mutex for malloc family functions. */
	affirm_msg(mutex_init(&malloc_mutex) == 0,
            "Failed to initialize mutex used in malloc library");

    /* Initialize mutex for thread status array, which contains all threads
     * in this library. */
	affirm_msg(mutex_init(&thr_status_mux) == 0,
            "Failed to initialize mutex used in thread library");

    /* Initialize hashmap to store thread status information */
    if (new_map(tid2thr_statusp, NUM_BUCKETS) < 0)
        return -1;

	/* Store root thread info on hashmap. */
	thr_status_t *tp = malloc(sizeof(thr_status_t));
	affirm(tp);
    memset(tp, 0, sizeof(thr_status_t));

	cond_t *exit_cvar = malloc(sizeof(exit_cvar));
	affirm(exit_cvar);
	affirm(cond_init(exit_cvar) == 0);

    tp->tid = gettid();
	tp->exit_cvar = exit_cvar;

    insert(tid2thr_statusp, tp);

	return 0;
}

/** @brief Creates a new thread to run func(arg)
 *
 *  To prevent overlaps of the create thread's stack memory and the stack
 *  memory of the initial thread, we malloc() a char array called thr_stack of
 *  THR_STACK_SIZE bytes. %ebp and %esp is set to thr_stack_high, which points to index
 *  THR_STACK_SIZE-1 of thr_stack, %eip is set to func
 *
 *  @param func Function to run in new thread
 *  @param arg Argument to pass into func
 *
 *  @return 0 on success, negative number on failure
 */
int
thr_create( void *(*func)(void *), void *arg )
{
	if (!THR_INITIALIZED) return THR_UNINIT;

	thr_status_t *child_tp = malloc(sizeof(thr_status_t));
	affirm(child_tp);
    memset(child_tp, 0, sizeof(thr_status_t));

	char *thr_stack = malloc(THR_STACK_SIZE);
	affirm(thr_stack);

	cond_t *exit_cvar = malloc(sizeof(exit_cvar));
	affirm(exit_cvar);
	affirm(cond_init(exit_cvar) == 0);

    /* Set child_tp values */
	child_tp->exit_cvar = exit_cvar;
	child_tp->thr_stack_low = thr_stack;

	// TODO why do we - ALIGN here thr_stack high is 1 + highest addressable
	// byte in the stack
	// highest writable
	// thr_stack_high is 1 + (highest accessible byte address in child stack)
	child_tp->thr_stack_high = thr_stack + THR_STACK_SIZE; // - ALIGN;

	//TODO install child handler test this

	assert(((uint32_t)child_tp->thr_stack_high) % ALIGN == 0);

    /* Get mutex to avoid conflicts between parent and child */
    mutex_lock(&thr_status_mux);

	int tid = thread_fork(child_tp->thr_stack_high, func, arg);

    /* In parent thread, update child information. */
    child_tp->tid = tid;

    insert(tid2thr_statusp, child_tp);

    /* Unlock after storing child_tp, so child may store its exit status */
    mutex_unlock(&thr_status_mux);

	return tid;
}

/** @brief Joins a diffent thread, collecting its exit status
 *         and cleaning up its resources. Blocks until thread exits.
 *
 *  @param tid ID of thread to join
 *  @param statusp Memory location where to store exit status.
 *
 *  @return 0 on success, negative number on failure
 */
int
thr_join( int tid, void **statusp )
{
    mutex_lock(&thr_status_mux);

	/* con_wait for thread with tid to exit */
    thr_status_t *thr_statusp;
	while (1) {
		/* If some other thread already cleaned up (or if that thread was
         * never created), do nothing more, return */
		if ((thr_statusp = get(tid2thr_statusp, tid)) == NULL) {
			mutex_unlock(&thr_status_mux);
			return -1;
		}
		/* If exited and not cleaned up, break and clean up */
		if (thr_statusp->exited)
			break;

        cond_wait(thr_statusp->exit_cvar, &thr_status_mux);
    }

    /* Collect exit status if statusp non-NULL */
    assert(thr_statusp->exited);
    if (statusp)
        *statusp = thr_statusp->status;

    /* Remove from hashmap, signaling we've cleaned up this thread */
    remove(tid2thr_statusp, tid);

    /* Free child stack and thread status and cond var */
    if (thr_statusp->thr_stack_low)
        free(thr_statusp->thr_stack_low);
    cond_destroy(thr_statusp->exit_cvar);
    free(thr_statusp->exit_cvar);
    free(thr_statusp);

    mutex_unlock(&thr_status_mux);

    return 0;
}

/** @brief Exit calling thread.
 *
 *  @param status Status with which to exit
 *
 *  @return Void
 */
void
thr_exit( void *status )
{
	int tid = gettid();

    mutex_lock(&thr_status_mux);

    thr_status_t *tp = get(tid2thr_statusp, tid);
    assert(tp);

    assert(tp->tid == tid);

    /* Store exit status for retrieval in thr_join */
    tp->exited = 1;
    tp->status = status;

	/* Tell all waiting joining threads of exit */
    cond_broadcast(tp->exit_cvar);

    mutex_unlock(&thr_status_mux);


    /* Exit thread */
    vanish();
}

/** @brief Yield to another thread.
 *
 *  @param tid Id of thread to yield to
 *
 *  @return Void
 */
int
thr_yield( int tid )
{
	return yield(tid);
}

// TODO anything less expensive than a full blown syscall?
// somehow cache this value?
/** @brief Get id of calling thread
 *
 *  @return Id of calling thread
 */
int
thr_getid( void )
{
	return gettid();
}




