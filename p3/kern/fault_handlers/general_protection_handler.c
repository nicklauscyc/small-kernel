/** @file general_protection_handler.c
 *  @brief Functions for alignment check faults
 */
#include <seg.h>	/* SEGSEL_KERNEL_CS */
#include <assert.h> /* panic() */
#include <simics.h>
#include <stdint.h> /* uint32_t */
void
general_protection_handler( uint32_t *ebp )
{
	int error_code = *(ebp + 1);
	int eip = *(ebp + 2);
	int cs = *(ebp + 3);
	int eflags = *(ebp + 4);
	int esp, ss;

	/* More args on stack if it was from user space */
	if (cs == SEGSEL_USER_CS) {
		esp = *(ebp + 5);
		ss = *(ebp + 6);
		affirm_msg(error_code == 0, "General protection fault while loading a "
	    	       "segment descriptor\n"
				   "error_code:0x%08x\n "
				   "eip:0x%08x\n "
				   "cs:0x%08x\n "
				   "eflags:0x%08x\n "
				   "esp:0x%08x\n "
				   "ss:0x%08x\n ",
				   error_code, eip, cs, eflags, esp, ss);
	} else {
		affirm_msg(error_code == 0, "General protection fault while loading a "
	    	       "segment descriptor\n"
				   "error_code:0x%08x\n "
				   "eip:0x%08x\n "
				   "cs:0x%08x\n "
				   "eflags:0x%08x\n ",
				   error_code, eip, cs, eflags);
	}
	if (cs == SEGSEL_KERNEL_CS) {
		panic("[Kernel mode] General protection fault encountered error at "
		        "0x%x.", eip);
	}
	/* TODO: acknowledge signal and call user handler  */

	panic("[User mode] General protection fault encountered at 0x%x", eip);
}
