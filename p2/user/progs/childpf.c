/** @file childpf.c
 *  @brief Test a child thread that page faults.
 *  @author Nicklaus Choo (nchoo)
 */

#include <assert.h> /* assert() */
#include <thread.h> /* thread() */
#include <simics.h> /* lprintf() */
#include <syscall.h> /* gettid() */


void *
child_pf(void *arg)
{
	lprintf("If Pagefault, test passed");
	int *bad_addr = (int *) 0x3333FFFF;
	*bad_addr = 0xdeadd00d;
	return bad_addr;
}

int main()
{

	assert(thr_init(PAGE_SIZE) == 0);
	int tid = thr_create(child_pf, 0);
	thr_join(tid, 0);
	lprintf("TEST_FAIL");

	return 0;
}




