/** @file pagefault_handler.c
 *  @brief Functions for page fault handling
 *  TODO write a macro for syscall handler/install handler wrappers
 *  @author Nicklaus Choo (nchoo)
 */
#include <assert.h> /* panic() */
#include <install_handler.h> /* install_handler_in_idt() */
#include <x86/cr.h> /* get_cr2() */

/** @brief Installs the pagefault_handler() interrupt handler
 *
 *  @param idt_entry Index in IDT to install to
 *  @param asm_wrapper Assembly wrapper to call pagefault_handler
 *  @return 0 on success, -1 on error.
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

/** @brief Prints out the offending address on and calls panic()
 *
 *  @return Void.
 */
void
pagefault_handler( void )
{
	uint32_t faulting_vm_address = get_cr2();
	panic("Page fault at vm address:0x%lx!", faulting_vm_address);
}
