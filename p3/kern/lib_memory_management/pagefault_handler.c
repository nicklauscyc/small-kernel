/** @file pagefault_handler.c
 *  @brief Functions for page fault handling
 *  TODO write a macro for syscall handler/install handler wrappers
 *  @author Nicklaus Choo (nchoo)
 */

#include <cr.h>					/* get_cr2() */
#include <asm.h>				/* outb() */
#include <page.h>				/* PAGE_SIZE */
#include <assert.h>				/* panic() */
#include <memory_manager.h>		/* zero_page_pf_handler */
#include <install_handler.h>	/* install_handler_in_idt() */
#include <interrupt_defines.h>	/* INT_CTL_PORT, INT_ACK_CURRENT */

#include <simics.h>
#include <logger.h> /* log() */


#define PRESENT_BIT			(1 << 0)
#define READ_WRITE_BIT		(1 << 1)
#define USER_SUPERVISOR_BIT (1 << 2)
#define RESERVED_BIT_BIT	(1 << 3)

/** @brief Prints out the offending address on and calls panic()
 *
 *  @return Void.
 */
void
pagefault_handler( int error_code, int eip, int cs )
{
	/* Acknowledge interrupt immediately */
	outb(INT_CTL_PORT, INT_ACK_CURRENT);

	uint32_t faulting_vm_address = get_cr2();

	if (zero_page_pf_handler(faulting_vm_address) == 0) {
		return;
	}
	// TODO Add the swexn() execution here
	/* TODO: acknowledge signal and call user handler  */

	(void)cs;
	log_info("pagefault_handler(): error_code:0x%x, eip:0x%x, cs:0x%x", error_code,
	    eip, cs);

	char user_mode[] = "[USER-MODE]";
	char supervisor_mode[] = "[SUPERVISOR-MODE]";

	char *mode = (error_code & USER_SUPERVISOR_BIT) ?
		user_mode : supervisor_mode;

	if (!(error_code & PRESENT_BIT))
		panic("%s Page fault at vm address:0x%lx at instruction 0x%lx! %s",
				mode, faulting_vm_address, eip,
				faulting_vm_address < PAGE_SIZE ?
				"Null dereference." : "Page not present.");

	if (error_code & READ_WRITE_BIT)
		panic("%s Page fault at vm address:0x%lx at instruction 0x%lx! "
				"Writing into read-only page",
				mode, faulting_vm_address, eip);

	if (error_code & RESERVED_BIT_BIT)
		panic("%s Page fault at vm address:0x%lx at instruction 0x%lx! "
				"Writing into reserved bits",
				mode, faulting_vm_address, eip);
}
