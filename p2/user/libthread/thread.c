/** @file thread.c
 *
 *  @brief This file implements the Thread Management API declared in
 *         thread.h
 *
 *  @author Nicklaus Choo (nchoo)
 */

#include <simics.h> /* lprintf() */
#include <malloc.h> /* malloc() */
#include <thr_internals.h> /* thread_fork() */
#include <thread.h> /* all thread library prototypes */
#include <syscall.h> /* gettid() */
#include <assert.h> /* assert() */

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


/** @brief Struct containing all necesary information about a thread.
 */
typedef struct {
	char *thr_stack_low;
	char *thr_stack_high;
	int tid;
	int runnable;
} thr_status_t;

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
	if (size == 0) return -1;
	THR_STACK_SIZE = size;
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
	if (THR_STACK_SIZE == -1) return THR_UNINIT;

	/* Allocate memory for thread stack */
	//TODO magic number boi
	char *thr_stack = _malloc(THR_STACK_SIZE + 3);
	assert(thr_stack);

	/* Allocate memory for thr_status_t */
	thr_status_t *tp = _malloc(sizeof(thr_status_t));
	assert(tp);
	tp->thr_stack_low = thr_stack;
	tp->thr_stack_high = thr_stack + THR_STACK_SIZE + 3;
	assert(tp->thr_stack_high > tp->thr_stack_low);
	lprintf("before thread_fork() \n");

	/* Fork a new thread */
	MAGIC_BREAK;
	int tid = thread_fork();
 	lprintf("thread_fork() after tid: %d\n",gettid());
	MAGIC_BREAK;

	/* In child thread */
	if (tid == 0) {
 	lprintf("called thread_fork() success child\n");

		assert(tp);
		lprintf("thr_create child!\n");
		MAGIC_BREAK;
		assert(func);
		assert(arg);
		run_thread(tp->thr_stack_high, func, arg);

		/* On return call thr_exit() */
		thr_exit(0);

	/* In parent thread */
	} else {
lprintf("called thread_fork() success parent\n");


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
	return 0;
}

void
thr_exit(void *status)
{
	int tid = gettid();
	lprintf("thr_exit() called from tid: %d\n", tid);

	/* Remove from array */
	/* TODO protect with mutex */
	int i = 0;
	while (thr_arr[i]->tid != tid) i++;
	thr_status_t *tp = thr_arr[i];
	free(tp->thr_stack_low);

	/* Deschedule */
	int x = 0;
	deschedule(&x);
}

int
thr_getid(void)
{
	return gettid();
}




