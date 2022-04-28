/** @file pagefault_handler.c
 *  @brief Functions for page fault handling
 *  @author Nicklaus Choo (nchoo)
 */

#include <cr.h>					/* get_cr2() */
#include <asm.h>				/* outb() */
#include <seg.h>				/* SEGSEL_USER_CS */
#include <page.h>				/* PAGE_SIZE */
#include <ureg.h>				/* SWEXN_CAUSE_ */
#include <swexn.h>				/* handle_exn */
#include <assert.h>				/* panic() */
#include <scheduler.h>			/* get_running_tid */
#include <common_kern.h>		/* USER_MEM_START  */
#include <panic_thread.h>		/* panic_thread() */
#include <memory_manager.h>		/* zero_page_pf_handler */
#include <install_handler.h>	/* install_handler_in_idt() */
#include <interrupt_defines.h>	/* INT_CTL_PORT, INT_ACK_CURRENT */


#include <simics.h>
#include <logger.h> /* log() */
#include <lib_life_cycle/life_cycle.h>

/* Page fault error code flag definitions */

/** @brief if 0, the fault was caused by a non-present page
 *         if 1, the fault was caused by a page-level protection violation
 */
#define P_BIT    (1 << 0)

/** @brief if 0, the access causing the fault was a read
 *         if 1, the access causing the fault was a write
 */
#define WR_BIT   (1 << 1)

/** @brief if 0, the access causing the fault originated when the processor
 *               was executing in supervisor mode
 *         if 1, the access causing the fault originated when the processor
 *               was executing in user mode
 */
#define US_BIT   (1 << 2)

/** @brief if 0, the fault was not caused by reserved bit violation
 *         if 1, the fault was caused by reserved bits set to 1 in a
 *               page directory
 */
#define RSVD_BIT (1 << 3)


/** @brief Prints out the offending address on and calls panic().
 *
 *  Will invoke the zero page pagefault handler that implements ZFOD.
 *
 *  @param ebp The base pointer from which we can access the other arguments
 *             put on the handler stack by the processor.
 *  @return Void.
 */
void
pagefault_handler( int *ebp )
{
	int error_code = *(ebp + 1);
	int eip = *(ebp + 2);
	int cs = *(ebp + 3);
	int eflags = *(ebp + 4);
	int esp, ss;

	/* More args on stack if it was from user space */
	if (cs == SEGSEL_USER_CS) {
		esp = *(ebp + 5);
		ss = *(ebp + 6);
	}
	(void) esp;
	(void) ss;

	uint32_t faulting_vm_address = get_cr2();

	char user_mode[] = "[USER-MODE]";
	char supervisor_mode[] = "[SUPERVISOR-MODE]";
	char *mode = (error_code & US_BIT) ?
		user_mode : supervisor_mode;

	/* If the fault happened while we were running in kernel AKA supervisor
	 * mode and accessing kernel memory, something really bad happened and
	 * we need to crash.
	 */
	if ((error_code & US_BIT) == 0) {

		/* This case is for when we are in kernel mode and configuring the
		 * user stack which was allocated by new_pages */
		if ( faulting_vm_address >= USER_MEM_START
			&& (error_code & WR_BIT)) {
			if (zero_page_pf_handler(faulting_vm_address) == 0) {
				return;
			}
		}
		panic("pagefault_handler(): %s "
		      "pagefault while running in kernel mode! "
 	          "error_code:0x%x "
              "eip:0x%x "
              "cs:0x%x "
 	          "faulting_vm_address:%p",
              mode, error_code, eip, cs, faulting_vm_address);
	}

	/* If not a kernel exception, acknowledge interrupt */
	outb(INT_CTL_PORT, INT_ACK_CURRENT);

	/* If the fault happened because reserved bits were accidentally set
	 * in the page directory, the page directory is corrupted and we need to
	 * crash. */
	if (error_code & RSVD_BIT) {

		/* Get offending page directory entry */
		uint32_t pd_index = PD_INDEX(faulting_vm_address);
		uint32_t **pd = (uint32_t **) TABLE_ADDRESS(get_cr3());
		affirm(pd);
		uint32_t *pd_entry = pd[pd_index];

		panic("pagefault_handler(): %s "
		      "pagefault due to corrupted page directory entry (pd_entry) "
			  "reserved bits "
 	          "error_code:0x%x "
              "eip:0x%x "
              "cs:0x%x "
 	          "faulting_vm_address:%p "
			  "pd_entry:%p ",
              mode, error_code, eip, cs, faulting_vm_address, pd_entry);
	}

	/* Page not present */
	if (!(error_code & P_BIT)) {
		handle_exn(ebp, SWEXN_CAUSE_PAGEFAULT, faulting_vm_address);
		panic_thread("%s Page fault at vm address:0x%lx at instruction 0x%lx! "
		             "%s",
					 mode, faulting_vm_address, eip,
                     faulting_vm_address < PAGE_SIZE ?
					 "Null dereference." : "Page not present.");
	}

	/* Protection violation */
	if (cs == SEGSEL_USER_CS && (eip < USER_MEM_START
				|| faulting_vm_address < USER_MEM_START)) {
		handle_exn(ebp, SWEXN_CAUSE_PAGEFAULT, faulting_vm_address);
		panic_thread("%s Page fault at vm address:0x%lx at instruction 0x%lx! "
					"User mode trying to access kernel memory",
					mode, faulting_vm_address, eip);
	}

	/* Fault was a write and page was present and in user memory */
	if (error_code & WR_BIT) {
		/* Check if this was a ZFOD allocated page, since those pages are
		 * marked as read-only in the page directory */
		if (zero_page_pf_handler(faulting_vm_address) == 0) {
			return;
		}
		handle_exn(ebp, SWEXN_CAUSE_PAGEFAULT, faulting_vm_address);
		panic_thread("%s Page fault at vm address:0x%lx at instruction 0x%lx! "
					"Writing into read-only page",
					mode, faulting_vm_address, eip);
	}

	/* NOTREACHED */

	panic("PAGEFAULT HANDLER unknown crash reason!\n "
				   "error_code:0x%08x\n "
				   "eip:0x%08x\n "
				   "cs:0x%08x\n "
				   "eflags:0x%08x\n "
				   "faulting_vm_address: 0x%08lx",
				   error_code, eip, cs, eflags, faulting_vm_address);


}
