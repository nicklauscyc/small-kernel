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

// TODO locking
int
new_pages( void *base, int len )
{
	assert(is_valid_pd(get_tcb_pd(get_running_thread())));

	/* Acknowledge interrupt immediately */
    outb(INT_CTL_PORT, INT_ACK_CURRENT);

	log("base:%p, len:0x%08lx", base, len);

	if ((uint32_t)base < USER_MEM_START) {
		log_warn("base < USER_MEM_START");
		return -1;
	}
	if (!PAGE_ALIGNED(base)) {
		log_warn("base not page aligned!");
		return -1;
	}
	if (len <= 0) {
		log_warn("len <= 0!");
		return -1;
	}
	if (len % PAGE_SIZE != 0) {
		log_warn("len is not a multiple of PAGE_SIZE!");
		return -1;
	}
	/* Check if any portion is currently allocated in task address space */
	char *base_char = (char *) base;
	for (uint32_t i = 0; i < len; ++i) {
		if (is_user_pointer_allocated(base_char + i)) {
			log_warn("%p is already allocated!", base_char + i);
			return -1;
		}
	}
	/* Allocate a zero frame to each PAGE_SIZE region of memory */
	int res = 0;
	for (uint32_t i = 0; i < len / PAGE_SIZE; ++i) {
		assert(res == 0);
		res += allocate_user_zero_frame((void *)TABLE_ADDRESS(get_cr3()),
		                                (uint32_t) base + (i * PAGE_SIZE));

		/* If any step fails, unallocate zero frame, return -1 */
		if (res < 0) {
			log_warn("unable to allocate zero frame in new_pages()");

			/* Cleanup */
			for (uint32_t j = 0; j < i; ++j) {
				unallocate_user_zero_frame((void *)TABLE_ADDRESS(get_cr3()),
		                                   (uint32_t) base + (j * PAGE_SIZE));
			}
			return -1;
		}
	}
	return res;
}
