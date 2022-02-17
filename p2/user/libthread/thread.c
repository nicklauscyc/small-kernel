/** @file thread.c
 *
 *  @brief This file implements the Thread Management API declared in
 *         thread.h
 *
 *  @author Nicklaus Choo (nchoo)
 */

#include <malloc.h> /* malloc() */
#include <thr_internals.h> /* thread_fork() */
#include <thread.h> /* all thread library prototypes */
#include <syscall.h> /* gettid() */
#include <assert.h> /* assert() */
#include <mutex.h> /* mutex_t */

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
mutex_t *mmp; /* global mutex for malloc family functions */

/* Global variable to indicate that thr library has been initialized */
// FIXME: Might not need this as programs are well behaved (section 5.3)
int THR_INITIALIZED = 0;


/* TODO make this unbounded */
#define NUM_THREADS 20
static thr_status_t *thr_arr[NUM_THREADS];

/** @brief Initializes the size all thread stacks will have if the thread
 *         is not the initial thread.
 *
 *  @param size Stack size for thread in bytes
 *  @return 0 if successful, -1 if size <= 0
 */
int
thr_init(unsigned int size)
{
	/* Initialize the mutex for malloc family functions. After this only
	 * malloc() without underscores should be called */
	mmp = _malloc(((sizeof(mutex_t)/16)+1)*16);
	assert(mutex_init(mmp) == 0);

	if (size == 0 || THR_INITIALIZED) return -1;
	THR_STACK_SIZE = size;
    THR_INITIALIZED = 1;
	return 0;
}

/** @brief Creates a new thread to run func(arg)
 *
 *  To prevent overlaps of the create thread's stack memory and the stack
 *  memory of the initial thread, we malloc() a char array called thr_stack of
 *  THR_STACK_SIZE bytes. %ebp and %esp is set to thr_stack_high, which points to index
 *  THR_STACK_SIZE-1 of thr_stack, %eip is set to func
 */
int
thr_create(void *(*func)(void *), void *arg)
{
	/* thr_init() was not called prior, return error */
	if (!THR_INITIALIZED) return THR_UNINIT;

	/* Allocate memory for thread stack */
	//TODO magic number boa
	// Round the stack size up to a multiple of 4 bytes
	unsigned int ROUND_UP_THR_STACK_SIZE =
		(THR_STACK_SIZE / 16) * 16;
	if (THR_STACK_SIZE % 16 != 0)
		ROUND_UP_THR_STACK_SIZE += 1;

	assert(ROUND_UP_THR_STACK_SIZE % 4 == 0);
	char *thr_stack = malloc(ROUND_UP_THR_STACK_SIZE);
	assert(thr_stack);

	affirm_msg(thr_stack, "Failed to allocate child stack.");

	/* Allocate memory for thr_status_t */
	thr_status_t *tp = malloc(((sizeof(thr_status_t) / 16) + 1) * 16);
	assert(tp);

	affirm_msg(tp, "Failed to allocate memory for thread status.");
	tp->thr_stack_low = thr_stack;
	tp->thr_stack_high = thr_stack + THR_STACK_SIZE - 4;
	assert (((uint32_t)tp->thr_stack_high) % 4 == 0);

	/* Fork a new thread */
	int tid = thread_fork(tp->thr_stack_high, func, arg);

	/* In child thread */
	if (tid == 0) {
		int CHILD_RETURNED = 0;
        assert(CHILD_RETURNED); // Child thread should never return

	/* In parent thread */
	} else {
		/* Set remaining fields of tp */
		assert(tp);
		tp->tid = tid;
		tp->runnable = 1;

		/* Add to array of running threads */
	 	/* TODO protect this with mutex */
		int i = 0;
		while (thr_arr[i] == NULL) i++;
		thr_arr[i] = tp;
	}
	return tid;
}

// TODO: Use wait to collect tid of vanish(ed) child, then collect status
//int thr_join( int tid, void **statusp );

void
thr_exit(void *status)
{
	//int tid = gettid();

    // TODO: Store status somewhere so it may be retrieved
    // by a subsequent call to join

	/* Remove from array */
	/* TODO protect with mutex */
	//int i = 0;
	//while (thr_arr[i]->tid != tid) i++;
	//thr_status_t *tp = thr_arr[i];
	//free(tp->thr_stack_low);
	//thr_arr[i] = 0;

    vanish();
}

int
thr_getid(void)
{
	return gettid();
}




