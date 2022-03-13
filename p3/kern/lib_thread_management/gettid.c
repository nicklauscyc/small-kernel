/** @file gettid.c
 *  @brief Contains gettid interrupt handler and helper functions for
 *  	   installation
 *
 *
 */

#include <x86/asm.h> /* idt_base() */
#include <install_handler.h> /* install_handler_in_idt() */

/** @brief Installs the gettid() interrupt handler
 */
int
install_gettid_handler(int idt_entry, asm_wrapper_t *asm_wrapper)
{
	if (!asm_wrapper) {
		return -1;
	}
	init_gettid();
	return install_handler_in_idt(int idt_entry, asm_wrapper);
}



