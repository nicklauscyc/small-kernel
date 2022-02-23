/** @file join_root.c
 *  @brief Test the joining the root thread library API functions
 *  @author Nicklaus Choo (nchoo)
 */

#include <assert.h> /* assert() */
//#include "../../410user/inc/thread.h" /* thread() */
#include <thread.h> /* thread() */

#include <simics.h> /* lprintf() */
#include <syscall.h> /* gettid() */


void *
add_one(void *root_tid)
{
	tprintf("running child sees root_tid: %d", *((int *) root_tid));

	int x = 0;
	int *status;
	*status = x;
    thr_join(*(int *)root_tid, (void **) &status);

	assert(x == 69);
	thr_exit(0);
	return 0;
}

int main()
{
	assert(thr_init(PAGE_SIZE) == 0);

	int root_tid = gettid();
	int tid = thr_create(add_one, &root_tid);
	lprintf("created thread %d", tid);

	int y = 69;
	thr_exit(&y);


	return 0;
}




