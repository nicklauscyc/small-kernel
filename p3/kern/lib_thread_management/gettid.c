/** @file gettid.c
 *  @brief Contains gettid interrupt handler and helper functions for
 *  	   installation
 *
 *
 */
#include <scheduler.h>
#include <assert.h>
#include <x86/asm.h> /* idt_base() */
#include <install_handler.h> /* install_handler_in_idt() */
#include <x86/interrupt_defines.h> /* INT_CTL_PORT, INT_ACK_CURRENT */

/* For quick and dirty testing */
#include <lib_thread_management/mutex.h>
#include <simics.h>

void
init_gettid( void )
{
	/* Nothing to init */
}

int
gettid( void )
{
    /* Acknowledge interrupt and return */
    outb(INT_CTL_PORT, INT_ACK_CURRENT);

    return get_running_tid();
}

/** @brief Installs the gettid() interrupt handler
 */
int
install_gettid_handler(int idt_entry, asm_wrapper_t *asm_wrapper)
{
	if (!asm_wrapper) {
		return -1;
	}
	init_gettid();
	int res = install_handler_in_idt(idt_entry, asm_wrapper, DPL_3);
	return res;
}



