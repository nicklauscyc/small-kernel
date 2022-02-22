/** @file thread.c
 *
 *  @brief This file implements the Thread Management API declared in
 *         thread.h
 *
 *  @author Nicklaus Choo (nchoo)
 *  @author Andre Nascimento (anascime)
 *
 *  FIXME: What should we do about root thread's stack when joining it?
 *  Just don't free?
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

#define THR_UNINIT -2
/* Private global variable for non initial threads' stack size */
static unsigned int THR_STACK_SIZE = 0;

extern volatile void *global_stack_low; /* In autostack.c */
mutex_t malloc_mutex; /* global mutex for malloc family functions */

/* Global variable to indicate that thr library has been initialized */
// FIXME: Might not need this as programs are well behaved (section 5.3)
volatile int THR_INITIALIZED = 0;

hashmap_t tid2thr_status;
hashmap_t *tid2thr_statusp = &tid2thr_status;
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
	if (size == 0 || THR_INITIALIZED)
        return -1;

	/* Initializing mutexes should never fail as no memory allocation needed */
	/* Initialize mutex for malloc family functions, thread status hashtable. */
	affirm_msg(mutex_init(&malloc_mutex) == 0, "Failed to initialize mutex "
	           "used for threadsafe malloc library");
	affirm_msg(mutex_init(&thr_status_mux) == 0,
               "Failed to initialize mutex used for thread library");

	THR_STACK_SIZE = size;
    THR_INITIALIZED = 1;

    /* Initialize hashmap to store thread status information */
    init_map();

    /* Initialize cvar for root thread */
    memset(&root_exit_cvar, 0, sizeof(cond_t));
    if (cond_init(&root_exit_cvar) < 0)
		return -1;
	root_tstatusp->exit_cvar = &root_exit_cvar;

	/* Add root thread status to hashtable */
    insert(root_tstatusp);

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

	/* Allocate memory for thread stack and thread exception stack, round the
     * stack size up to a multiple of ALIGN bytes. */
    // TODO: Do we really need this?
	unsigned int ROUND_UP_THR_STACK_SIZE =
		((PAGE_SIZE + THR_STACK_SIZE + ALIGN - 1) / ALIGN) * ALIGN;

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

	affirm(cond_init(exit_cvar) == 0);
	child_tp->exit_cvar = exit_cvar;

	child_tp->thr_stack_low = thr_stack;

	// TODO why do we - ALIGN here thr_stack high is 1 + highest addressable
	// byte in the stack
	// highest writable
	// thr_stack_high is 1 + (highest accessible byte address in child stack)
	child_tp->thr_stack_high = thr_stack + THR_STACK_SIZE; // - ALIGN;

	//TODO install child handler test this

	assert(((uint32_t)child_tp->thr_stack_high) % ALIGN == 0);

    /* Get mutex so that child_tp is stored before child exits (and writes to it) */
    mutex_lock(&thr_status_mux);

	/* Run func on a new thread */ //TODO: How about fork_and_run instead?
	int tid = thread_fork(child_tp->thr_stack_high, func, arg);

    /* In parent thread, update child information. */
    child_tp->tid = tid;

    insert(child_tp);

    /* Only release lock after setting tid2thr_statusp[tid] so that
     * the child knows where to write its exit status. */
    mutex_unlock(&thr_status_mux);

	return tid;
}

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
    if (thr_statusp->thr_stack_low)
        free(thr_statusp->thr_stack_low);
    cond_destroy(thr_statusp->exit_cvar);
    free(thr_statusp->exit_cvar);
    free(thr_statusp);

    mutex_unlock(&thr_status_mux);

    return 0;
}

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

// TODO anything less expensive than a full blown syscall?
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




