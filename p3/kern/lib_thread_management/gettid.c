/** @file gettid.c
 *  @brief Contains gettid interrupt handler and helper functions for
 *  	   installation
 *
 *
 */

#include <assert.h>
#include <simics.h> /* lprintf() */
#include <x86/asm.h> /* idt_base() */
#include <install_handler.h> /* install_handler_in_idt() */
#include <x86/interrupt_defines.h> /* INT_CTL_PORT, INT_ACK_CURRENT */

void
init_gettid( void )
{
	/* honestly not sure what to init */
}

int
gettid(void )
{
	assert(0);
	lprintf("gettid called");
  /* Acknowledge interrupt and return */
  outb(INT_CTL_PORT, INT_ACK_CURRENT);
	lprintf("gettid ack ");

  return 0;
}

/** @brief Installs the gettid() interrupt handler
 */
int
install_gettid_handler(int idt_entry, asm_wrapper_t *asm_wrapper)
{
	lprintf("inside install_gettid_handler");
	if (!asm_wrapper) {
		return -1;
	}
	init_gettid();
	int res = install_handler_in_idt(idt_entry, asm_wrapper, DPL_0);
	lprintf("installed gettid, res:%d", res);
	return res;
}



