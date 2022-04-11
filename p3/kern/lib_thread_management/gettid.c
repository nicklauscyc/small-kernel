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




