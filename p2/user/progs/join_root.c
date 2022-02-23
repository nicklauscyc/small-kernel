/** @file join_root.c
 *  @brief est the joining the root thread library API functions
 *  @author Nicklaus Choo (nchoo)
 */

#include<malloc.h>
#include <assert.h> /* assert() */
//#include "../../410user/inc/thread.h" /* thread() */
#include <thread.h> /* thread() */

#include <simics.h> /* lprintf() */
#include <syscall.h> /* gettid() */


void *
add_one(void *root_tid)
{
	tprintf("running child sees root_tid: %d", *((int *) root_tid));

	void *status;
    thr_join(*(int *)root_tid, (void **) &status);

	assert(*(int *)status == 69);
	thr_exit(0);
	return 0;
}

int main()
{
	assert(thr_init(PAGE_SIZE) == 0);

	int root_tid = gettid();
	int tid = thr_create(add_one, &root_tid);
	lprintf("created thread %d", tid);

	int *yp = malloc(sizeof(int));
	*yp = 69;
	thr_exit(yp);


	return 0;
}




