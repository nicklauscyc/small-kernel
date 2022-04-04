/** @file yield.c
 *  @brief Contains yield interrupt handler and helper functions for
 *  	   installation
 *
 */

#include <asm.h>				/* outb() */
#include <stddef.h>				/* NULL */
#include <scheduler.h>			/* yield_execution() */
#include <install_handler.h>	/* install_handler_in_idt() */
#include <interrupt_defines.h>	/* INT_CTL_PORT, INT_ACK_CURRENT */

int
yield( int tid )
{
    /* Acknowledge interrupt immediately */
    outb(INT_CTL_PORT, INT_ACK_CURRENT);

	return yield_execution(NULL, RUNNABLE, tid);
}

/** @brief Installs the yield() interrupt handler
 */
int
install_yield_handler(int idt_entry, asm_wrapper_t *asm_wrapper)
{
	if (!asm_wrapper) {
		return -1;
	}
	int res = install_handler_in_idt(idt_entry, asm_wrapper, DPL_3);
	return res;
}
