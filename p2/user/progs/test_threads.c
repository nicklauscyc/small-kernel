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
	*iarg = *iarg + 1;
    return arg;
}

int main()
{
	assert(thr_init(PAGE_SIZE) == 0);

	int x = 1;
	int tid = thr_create(add_one, &x);

    void *out;
    thr_join(tid, &out);

    assert(*(int *)out == 2);

	return 0;
}




