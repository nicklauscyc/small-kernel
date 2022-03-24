/** @file pagefault_handler.c
 *  @brief Functions for page fault handling
 *
 *  @author Nicklaus Choo (nchoo)
 */
#include <assert.h> /* panic() */
#include <install_handler.h> /* install_handler_in_idt() */
#include <x86/cr.h> /* get_cr2() */

/** @brief Installs the fork() interrupt handler
 */
int
install_pf_handler(int idt_entry, asm_wrapper_t *asm_wrapper)
{
	if (!asm_wrapper) {
		return -1;
	}
	int res = install_handler_in_idt(idt_entry, asm_wrapper, DPL_0);
	return res;
}

void
pagefault_handler( void )
{
	uint32_t faulting_vm_address = get_cr2();
	panic("Page fault at vm address:0x%lx!", faulting_vm_address);
}
