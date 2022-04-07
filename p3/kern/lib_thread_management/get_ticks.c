/** @file get_ticks.c
 *  @brief Contains get_ticks interrupt handler
 */
#include <asm.h>				/* outb() */
#include <timer_driver.h>		/* get_total_ticks() */
#include <install_handler.h>	/* install_handler_in_idt() */
#include <interrupt_defines.h>	/* INT_CTL_PORT, INT_ACK_CURRENT */

int
get_ticks( void )
{
    /* Acknowledge interrupt and return */
    outb(INT_CTL_PORT, INT_ACK_CURRENT);

    return get_total_ticks();
}
