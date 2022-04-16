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
#include <memory_manager.h>		/* zero_page_pf_handler */
#include <install_handler.h>	/* install_handler_in_idt() */
#include <interrupt_defines.h>	/* INT_CTL_PORT, INT_ACK_CURRENT */


#include <simics.h>
#include <logger.h> /* log() */
#include <lib_life_cycle/life_cycle.h>

#define PRESENT_BIT			(1 << 0)
#define READ_WRITE_BIT		(1 << 1)
#define USER_SUPERVISOR_BIT (1 << 2)
#define RESERVED_BIT_BIT	(1 << 3)

/** @brief Prints out the offending address on and calls panic()
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
	log_info("pagefault_handler(): "
	         "error_code:0x%x "
			 "eip:0x%x "
			 "cs:0x%x "
	         "faulting_vm_address:%p",
			 error_code, eip, cs, faulting_vm_address);

	char user_mode[] = "[USER-MODE]";
	char supervisor_mode[] = "[SUPERVISOR-MODE]";

	char *mode = (error_code & USER_SUPERVISOR_BIT) ?
		user_mode : supervisor_mode;

	if (!(error_code & PRESENT_BIT)) {
		log_info("[tid %d] %s Page fault at vm address:0x%lx at instruction 0x%lx! %s",
				get_running_tid(), mode, faulting_vm_address, eip,
				faulting_vm_address < PAGE_SIZE ?
				"Null dereference." : "Page not present.");
		_vanish();
	}


	/* Jank check but reasonable for now */
	if (cs == SEGSEL_USER_CS && eip < USER_MEM_START) {
		log_info("[tid %d] %s Page fault at vm address:0x%lx at instruction 0x%lx! "
				"User mode trying to access kernel memory",
				get_running_tid(),mode, faulting_vm_address, eip);
		_vanish();
	}

	/* Check if this was a ZFOD allocated page */
	if (zero_page_pf_handler(faulting_vm_address) == 0) {
		return;
	}

	if (error_code & RESERVED_BIT_BIT) {
		log_info("[tid %d] %s Page fault at vm address:0x%lx at instruction 0x%lx! "
				"Writing into reserved bits",
				get_running_tid(), mode, faulting_vm_address, eip);
		_vanish();
	}
	if (error_code & READ_WRITE_BIT) {
		log_info("[tid %d] %s Page fault at vm address:0x%lx at instruction 0x%lx! "
				"Writing into read-only page",
				get_running_tid(), mode, faulting_vm_address, eip);
		_vanish();
	}
	MAGIC_BREAK;
	panic("PAGEFAULT HANDLER BROKEN!\n"
				   "error_code:0x%08x\n "
				   "eip:0x%08x\n "
				   "cs:0x%08x\n "
				   "eflags:0x%08x\n "
				   "faulting_vm_address: 0x%08lx",
				   error_code, eip, cs, eflags, faulting_vm_address);


}
