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
#include <lib_errors.h> /* EINVALIDARG, ETHR_LIB */

/* Align child thread stack to 4 bytes */
#define ALIGN 4

/* Private global variable for non initial threads' stack size */
static unsigned int THR_STACK_SIZE = 0;

mutex_t malloc_mutex; /* global mutex for malloc family functions */

/** @brief Indicates whether thr library has been initialized */
volatile int THR_INITIALIZED = 0;

/** @brief Map to store information from each existing thread */
hashmap_t tid2thr_status;
hashmap_t *tid2thr_statusp = &tid2thr_status;

/** @brief Mutex for thread status info. */
mutex_t thr_status_mux;


/** @brief Initializes the size all thread stacks will have if the thread
 *         is not the initial thread.
 *
 *  If thr_init() fails because of reasons beyond the user's control the user
 *  can still decide if they want to continue
 *  using a multithreaded environment or not.
 *
 *  @param size Stack size for thread in bytes, must be greater than  0
 *  @return 0 if successful, negative value on failure and THR_INITIALIZED set
 *          to 1.
 */
int
thr_init( unsigned int size )
{
	/* If size == 0 we want the user to know that this is an error so a
	 * subsequent calls to thr_create() will not surprise the user. If
	 * thr_init() has been previously called that is also an error since the
	 * user is supposed to call thr_init() exactly once
	 */
	if (size == 0 || THR_INITIALIZED)
        return -1;
	THR_STACK_SIZE = ((size + ALIGN - 1) / ALIGN) * ALIGN;

	/* Initializing mutexes should almost never fail as no memory allocation. */
	/* Initialize mutex for malloc family functions, thread status hashtable. */
	if (mutex_init(&malloc_mutex) < 0)
		return -1;

	if (mutex_init(&thr_status_mux) < 0) {
		tprintf("MUTEX DESTROYEDD FOR MALLOC");
		mutex_destroy(&malloc_mutex);
		return -1;
	}
    /* Initialize hashmap to store thread status information */
    init_map();

    /* Initialize cvar for root thread, add to hashtable */
	affirm(root_tstatusp == &root_tstatus);
    memset(root_tstatusp->exit_cvar, 0, sizeof(cond_t));
    if (cond_init(root_tstatusp->exit_cvar) < 0)
		return -1;
    insert(root_tstatusp);
	THR_INITIALIZED = 1;

	return 0;
}

/** @brief Creates a new thread to run func(arg)
 *
 *  Each child thread needs memory allocation for 2 things. The first is
 *  the child's stack itself, where we also extend the allocation size to
 *  have enough space for the child's exception handler; the second is
 *  the struct thr_status_t which contains information about the child thread.
 *  This is essentially a thread control block.
 *
 *  @param func Function to run in new thread
 *  @param arg Argument to pass into func
 *  @return 0 on success, negative number on failure
 */
int
thr_create( void *(*func)(void *), void *arg )
{
	if (!THR_INITIALIZED)
		return -1;

	/* Get total size of child thread stack and child exception stack */
	unsigned int ROUND_UP_THR_STACK_SIZE =
		((PAGE_SIZE + THR_STACK_SIZE + ALIGN - 1) / ALIGN) * ALIGN;

    /* Allocate child stack */
	char *thr_stack = calloc(1, ROUND_UP_THR_STACK_SIZE);
	if (!thr_stack)
		return -1;

	/* Allocate memory for child status struct thr_status_t */
	thr_status_t *child_tp = calloc(1, sizeof(thr_status_t));
	if (!child_tp)
		return -1;

	/* Set all fields for child_tp apart from tid which will be set later */
	child_tp->thr_stack_low = thr_stack;
	child_tp->thr_stack_high = thr_stack + THR_STACK_SIZE;
	child_tp->exited = 0;
	child_tp->status = 0;
	child_tp->exit_cvar = &(child_tp->_exit_cvar);
	if (cond_init(child_tp->exit_cvar) < 0)
		return -1;

    /* Get mutex to avoid conflicts between parent and child */
    mutex_lock(&thr_status_mux);

	/* In parent thread, update child tid, insert to hashtable. */
	int tid = thread_fork(child_tp->thr_stack_high, func, arg);
    child_tp->tid = tid;
    insert(child_tp);

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
		if ((thr_statusp = get(tid)) == NULL) {
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
    remove(tid);

    /* Free child stack and thread status and cond var */
    if (thr_statusp->thr_stack_low) {

		/* Deallocate stack of non-root thread */
		if (thr_statusp->thr_stack_low != global_stack_low) {
			free(thr_statusp->thr_stack_low);

		/* Deallocate stack of root thread */
		} else {
			tprintf("joining a root thread stack_low: %x", thr_statusp->thr_stack_low);
			affirm(thr_statusp->thr_stack_low == global_stack_low);
			if (remove_pages(thr_statusp->thr_stack_low) < 0) {
				tprintf("remove pages failed");
				mutex_unlock(&thr_status_mux);

				return -1;
			}
		}
	}
	tprintf("after clause");
    cond_destroy(thr_statusp->exit_cvar);
	tprintf("after cond_destroy");
    free(thr_statusp);

	tprintf("before mut unlock");
    mutex_unlock(&thr_status_mux);
	tprintf("after mut unlock");

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

    thr_status_t *tp = get(tid);
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




