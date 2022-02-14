/** @file thread.c
 *
 *  @brief This file implements the Thread Management API declared in
 *         thread.h
 *
 *  @author Nicklaus Choo (nchoo)
 */

#include <malloc.h> /* malloc() */

#define THR_UNINIT -2
/* Private global variable for non initial threads' stack size */
static unsigned int THR_STACK_SIZE = -1;

typedef struct {
	char *thr_stack_low;
	char *thr_stack_high;
	int tid;
	int runnable;
} thr_status_t;

/** @brief Initializes the size all thread stacks will have if the thread
 *         is not the initial thread.
 *
 *  @param size Stack size for thread in bytes
 *  @return 0 if successful, -1 if size <= 0
 */
int
thr_init(unsigned int size)
{
	if (size <= 0) return -1;
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
	char *thr_stack = malloc(THR_STACK_SIZE);

	/* Allocate memory for thr_status_t */

	return 0;

}

