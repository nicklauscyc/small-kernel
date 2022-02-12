/* If you want to use assembly language instead of C,
 * delete this autostack.c and provide an autostack.S
 * instead.
 */

#include <ureg.h>
#include <syscall.h> /* PAGE_SIZE */
#include <simics.h> /* lprintf() */
#include <stdint.h>  /* uint32_t */

/* intel-sys.pdf pg182 lowest bit P=0 if fault by non-present page, 1 o/w */
#define ERR_P_PROTECTION_VIOLATION_MASK 0x00000001


void install_autostack(void * stack_high, void * stack_low);
void pf_swexn_handler(void *arg, ureg_t *ureg);

/* TODO I guess this guy installs the handler? */
void
install_autostack(void *stack_high, void *stack_low)
{
	lprintf("install_autostack() stack_high: %p, stack_low: %p\n",
	        stack_high, stack_low);
  (void)stack_high;
  (void)stack_low;
}

/** @brief Page fault exception handler
 */
void pf_swexn_handler(void *arg, ureg_t *ureg)
{
	/* Get relevant info */
	int cause = ureg->cause;

	/* Memory address resulting in fault */
	unsigned int cr2 = ureg->cr2;

	/* Why memory address was inaccessible */
	unsigned int error_code = ureg->error_code;

	// TODO below not needed?
	// unsigned int eip = ureg->eip;
	//
	lprintf("pf_swexn_handler() cause: %x, cr2: %x, error_code: %x\n",
	        cause, cr2, error_code);

	/* Only deal with pagefault execptions */
	if (cause != SWEXN_CAUSE_PAGEFAULT) return;

	/* Only deal with faults caused by non-present page */
	if (!(ERR_P_PROTECTION_VIOLATION_MASK & error_code)) {
		new_pages((void *) (uint32_t) cr2, PAGE_SIZE);
	}
}

