/** @file pagefault_handler.c
 *  @brief Functions for page fault handling
 *  TODO write a macro for syscall handler/install handler wrappers
 *  @author Nicklaus Choo (nchoo)
 */

#include <cr.h>					/* get_cr2() */
#include <asm.h>				/* outb() */
#include <seg.h>				/* SEGSEL_USER_CS */
#include <page.h>				/* PAGE_SIZE */
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
 *  @return Void.
 */
void
pagefault_handler( uint32_t *ebp )
{
	/* Acknowledge interrupt immediately */
	outb(INT_CTL_PORT, INT_ACK_CURRENT);

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

	// TODO Add the swexn() execution here
	/* TODO: acknowledge signal and call user handler  */

	(void)cs;

	char user_mode[] = "[USER-MODE]";
	char supervisor_mode[] = "[SUPERVISOR-MODE]";
	char *mode = (error_code & US_BIT) ?
		user_mode : supervisor_mode;

	/* If the fault happened while we were running in kernel AKA supervisor
	 * mode, something really bad happened and we need to crash
	 */
	if ((error_code & US_BIT) == 0) {
		MAGIC_BREAK;
		panic("pagefault_handler(): %s "
		      "pagefault while running in kernel mode! "
 	          "error_code:0x%x "
              "eip:0x%x "
              "cs:0x%x "
 	          "faulting_vm_address:%p",
              mode, error_code, eip, cs, faulting_vm_address);
	}
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

	/* Fault was a write */
	if (error_code & WR_BIT) {
		/* Check if this was a ZFOD allocated page, since those pages are
		 * marked as read-only in the page directory */
		if (zero_page_pf_handler(faulting_vm_address) == 0) {
			return;
		}
		panic_thread("%s Page fault at vm address:0x%lx at instruction 0x%lx! "
					"Writing into read-only page",
					mode, faulting_vm_address, eip);
	} else {

		panic_thread("%s Page fault at vm address:0x%lx at instruction 0x%lx! "
					"reading permissions wrong",
					 mode, faulting_vm_address, eip);

	}


	if (!(error_code & P_BIT)) {
		panic_thread("%s Page fault at vm address:0x%lx at instruction 0x%lx! %s",
					mode, faulting_vm_address, eip,
					faulting_vm_address < PAGE_SIZE ?
					"Null dereference." : "Page not present.");
	} else {

		panic_thread("%s Page fault at vm address:0x%lx at instruction 0x%lx! %s",
					mode, faulting_vm_address, eip,
					faulting_vm_address < PAGE_SIZE ?
					"Null dereference." : "page-level protection violation.");
	}

	if (cs == SEGSEL_USER_CS && eip < USER_MEM_START) {
		panic_thread("[tid %d] %s Page fault at vm address:0x%lx at instruction 0x%lx! "
					"User mode trying to access kernel memory",
					get_running_tid(),mode, faulting_vm_address, eip);
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
