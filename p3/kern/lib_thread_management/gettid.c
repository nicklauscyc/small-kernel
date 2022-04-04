/** @file gettid.c
 *  @brief Contains gettid interrupt handler and helper functions for
 *  	   installation
 *
 *
 */
#include <scheduler.h>			/* get_running_tid() */
#include <asm.h>				/* outb() */
#include <install_handler.h>	/* install_handler_in_idt() */
#include <interrupt_defines.h>	/* INT_CTL_PORT, INT_ACK_CURRENT */

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
	int res = install_handler_in_idt(idt_entry, asm_wrapper, DPL_3);
	return res;
}



