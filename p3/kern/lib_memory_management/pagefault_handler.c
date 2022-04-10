/** @file pagefault_handler.c
 *  @brief Functions for page fault handling
 *  TODO write a macro for syscall handler/install handler wrappers
 *  @author Nicklaus Choo (nchoo)
 */
#include <assert.h> /* panic() */
#include <install_handler.h> /* install_handler_in_idt() */
#include <x86/cr.h> /* get_cr2() */
#include <memory_manager.h>
#include <simics.h>
#include <x86/asm.h>   /* outb() */
#include <x86/interrupt_defines.h> /* INT_CTL_PORT, INT_ACK_CURRENT */

/** @brief Prints out the offending address on and calls panic()
 *
 *  @return Void.
 */
void
pagefault_handler( void )
{
	/* Acknowledge interrupt immediately */
	outb(INT_CTL_PORT, INT_ACK_CURRENT);

	uint32_t faulting_vm_address = get_cr2();

	if (zero_page_pf_handler(faulting_vm_address) == 0) {
		MAGIC_BREAK;
		return;
	}
	panic("Page fault at vm address:0x%lx!", faulting_vm_address);
}
