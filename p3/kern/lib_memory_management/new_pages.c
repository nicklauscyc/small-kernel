/** @file new_pages.c
 *  @brief new_pages syscall handler
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
#include <memory_manager_internal.h>


// TODO locking
int
new_pages( void *base, int len )
{
    assert(is_valid_pd(get_tcb_pd(get_running_thread())));

    /* Acknowledge interrupt immediately */
    outb(INT_CTL_PORT, INT_ACK_CURRENT);

    log("new_pages(): "
        "base:%p, len:0x%08lx", base, len);

    if ((uint32_t)base < USER_MEM_START) {
        log_info("new_pages(): "
                 "base < USER_MEM_START");
        return -1;
    }
    if (!PAGE_ALIGNED(base)) {
        log_info("new_pages(): "
                 "base not page aligned!");
        return -1;
    }
    if (len <= 0) {
        log_info("new_pages(): "
                 "len <= 0!");
        return -1;
    }
    if (len % PAGE_SIZE != 0) {
        log_info("new_pages(): "
                 "len is not a multiple of PAGE_SIZE!");
        return -1;
    }
    /* Check if enough frames to fulfill request */
    uint32_t pages_to_alloc = len / PAGE_SIZE;
    if (num_free_phys_frames() < pages_to_alloc) {
        log_info("new_pages(): "
                 "not enough free frames to satisfy request!");
        return -1;
    }
    /* Check if any portion is currently allocated in task address space */
    char *base_char = (char *) base;
    for (uint32_t i = 0; i < len; ++i) {
        if (is_user_pointer_allocated(base_char + i)) {
            log_info("new_pages(): "
                     "%p is already allocated!", base_char + i);
            return -1;
        }
    }
    /* Allocate a zero frame to each PAGE_SIZE region of memory */
    int res = 0;
    for (uint32_t i = 0; i < len / PAGE_SIZE; ++i) {
        assert(res == 0);

		/* We mark in the page table if the allocated page is the first */
		if (i == 0) {
			res += allocate_user_zero_frame((void *)TABLE_ADDRESS(get_cr3()),
											(uint32_t) base + (i * PAGE_SIZE),
											NEW_PAGE_BASE_FLAG);
		} else {
			res += allocate_user_zero_frame((void *)TABLE_ADDRESS(get_cr3()),
											(uint32_t) base + (i * PAGE_SIZE),
											NEW_PAGE_CONTINUE_FROM_BASE_FLAG);
		}
        /* If any step fails, unallocate zero frame, return -1 */
        if (res < 0) {
            log_info("new_pages(): "
                     "unable to allocate zero frame");

            /* Cleanup */
            for (uint32_t j = 0; j < i; ++j) {
                unallocate_user_zero_frame((void *)TABLE_ADDRESS(get_cr3()),
                                           (uint32_t) base + (j * PAGE_SIZE));
            }
            return -1;
        }
    }
	/* TODO jank get and set cr3() to flush TLB entries */
	set_cr3(get_cr3());
    return res;
}
