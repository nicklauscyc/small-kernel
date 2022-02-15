/** @file test-threads.c
 *  @brief Test the thread library API functions
 *  @author Nicklaus Choo (nchoo)
 */

#include <assert.h> /* assert() */
//#include "../../410user/inc/thread.h" /* thread() */
#include <thread.h> /* thread() */

#include <simics.h> /* lprintf() */
#include <syscall.h> /* gettid() */


void *
add_one(void *arg)
{
	int *iarg = (int *) arg;
	lprintf("hello from child thread with id: %d, *arg: %d\n", thr_getid(), *iarg);
	assert(thr_getid() == gettid());
	*iarg = *iarg + 1;
	return arg;
}

int main()
{
	int res = 0;
	res = thr_init(0);
	lprintf("res: %d\n", res);
	assert(res == -1);
	assert(thr_init(PAGE_SIZE) == 0);

	int x = 1;
	int *arg = &x;
	*arg = 1;
	assert(*arg == 1);
	int tid = thr_create(add_one,arg);
	lprintf("hello from parent thread, child id is: %d\n", tid);
	sleep(10);
	lprintf("*arg: %d\n", *arg);
	assert(*arg == 2);
	return 0;
}




