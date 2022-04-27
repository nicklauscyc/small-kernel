/** @file remove_pages.c
 *  @brief remove_pages syscall handler
 *
 *  @author Nicklaus Choo (nchoo)
 */

#include <common_kern.h>/* USER_MEM_START */
#include <x86/cr.h>
#include <logger.h>
#include <memory_manager.h>
#include <assert.h>
#include <task_manager.h>
#include <scheduler.h>
#include <x86/asm.h>   /* outb() */
#include <x86/interrupt_defines.h> /* INT_CTL_PORT, INT_ACK_CURRENT */
#include <page.h> /* PAGE_SIZE */
#include <simics.h>
#include <physalloc.h>
#include <memory_manager.h>
#include <memory_manager_internal.h>
#include <lib_thread_management/mutex.h> /* mutex_t */


int
remove_pages( void *base )
{
	uint32_t **pd = get_tcb_pd(get_running_thread());
    assert(is_valid_pd(pd));

    /* Acknowledge interrupt immediately */
    outb(INT_CTL_PORT, INT_ACK_CURRENT);

    if ((uint32_t)base < USER_MEM_START) {
        log_info("remove_pages(): "
                 "base < USER_MEM_START");
        return -1;
    }
    if (!PAGE_ALIGNED(base)) {
        log_info("remove_pages(): "
                 "base not page aligned!");
        return -1;
    }

	mutex_lock(&pages_mux);

	/* Check if base is legitimately in page table. Since page table is
	 * valid, if ptep is NULL then base was not even allocated */
	uint32_t *ptep = get_ptep((const uint32_t **) pd, (uint32_t) base);
	if (!ptep) {
		log_info("remove_pages(): "
				 "unable to get page table entry pointer:");
		mutex_unlock(&pages_mux);
		return -1;
	}
	/* Check if base was allocated by previous call to new_pages() */
	int sys_prog_flag = SYS_PROG_FLAG(*ptep);
	assert(is_valid_sys_prog_flag(sys_prog_flag));

	if (sys_prog_flag != NEW_PAGE_BASE_FLAG) {
		log_info("remove_pages(): "
				 "base:%p not previously allocated by new_pages(), "
				 "sys_prog_flag:0x%08x",
				 base, sys_prog_flag);
		mutex_unlock(&pages_mux);
		return -1;
	}
	/* Free the first frame */
	uint32_t curr = (uint32_t) base;
	unallocate_frame(pd, curr);
	curr += PAGE_SIZE;
	assert(is_valid_pd(pd));
	affirm(PAGE_ALIGNED(curr));

	/* Free remaining frames */
	ptep = get_ptep((const uint32_t **) pd, curr);
	while (ptep && (SYS_PROG_FLAG(*ptep) == NEW_PAGE_CONTINUE_FROM_BASE_FLAG)) {
		unallocate_frame(pd, curr);
		curr += PAGE_SIZE;
		assert(is_valid_pd(pd));
		affirm(PAGE_ALIGNED(curr));

		ptep = get_ptep((const uint32_t **) pd, curr);
	}
	log("remove_pages(): "
			 "unallocated base:%p, len:%d", base,
			 curr - ((uint32_t) base));

	mutex_unlock(&pages_mux);
    return 0;
}
