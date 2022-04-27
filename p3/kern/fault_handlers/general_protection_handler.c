/** @file general_protection_handler.c
 *  @brief Functions for alignment check faults
 */
#include <seg.h>			/* SEGSEL_KERNEL_CS */
#include <asm.h>			/* outb() */
#include <ureg.h>			/* SWEXN_CAUSE_ */
#include <swexn.h>			/* handle_exn */
#include <assert.h>			/* panic() */
#include <stdint.h>			/* uint32_t */
#include <panic_thread.h>	/* panic_thread() */
#include <interrupt_defines.h> /* INT_CTL_PORT, INT_ACK_CURRENT */

void
general_protection_handler( int *ebp )
{
	int error_code = *(ebp + 1);
	int eip = *(ebp + 2);
	int cs = *(ebp + 3);
	int eflags = *(ebp + 4);
	int esp, ss;

	/* More args on stack if it was from user space */
	if (cs == SEGSEL_KERNEL_CS) {
		panic("[Kernel mode] General protection fault encountered error at "
	    	       "segment descriptor\n"
				   "error_code:0x%08x\n "
				   "eip:0x%08x\n "
				   "cs:0x%08x\n "
				   "eflags:0x%08x\n ",
				   error_code, eip, cs, eflags);

	}

	/* User mode error */
	esp = *(ebp + 5);
	ss = *(ebp + 6);

	/* If not a kernel exception, acknowledge interrupt */
	outb(INT_CTL_PORT, INT_ACK_CURRENT);

	handle_exn(ebp, SWEXN_CAUSE_PROTFAULT, 0);
#include <simics.h>
	MAGIC_BREAK;

	panic_thread("Unhandled general protection fault while loading a "
			   "segment descriptor\n"
			   "error_code:0x%08x\n "
			   "eip:0x%08x\n "
			   "cs:0x%08x\n "
			   "eflags:0x%08x\n "
			   "esp:0x%08x\n "
			   "ss:0x%08x\n ",
			   error_code, eip, cs, eflags, esp, ss);
}
