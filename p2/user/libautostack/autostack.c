/* If you want to use assembly language instead of C,
 * delete this autostack.c and provide an autostack.S
 * instead.
 */

#include <ureg.h>
#include <syscall.h> /* PAGE_SIZE */
#include <simics.h> /* lprintf() */
#include <stdint.h>  /* uint32_t */
#include <stddef.h> /* NULL */

/* intel-sys.pdf pg182 lowest bit P=0 if fault by non-present page, 1 o/w */
#define ERR_P_PROTECTION_VIOLATION_MASK 0x00000001

/* Word size is 16 bit so 2 bytes */
#define WORD_SIZE 2


void install_autostack(void * stack_high, void * stack_low);
void pf_swexn_handler(void *arg, ureg_t *ureg);
void reg_pf_swexn_handler(void *stack_low);

/** @brief Installs pagefault handler at a region of memory that will never
 *         be touched by main() and all other function calls descending from
 *         main()
 *
 *   We allocate memory at address stack_low + PAGE_SIZE so that userspace
 *   execution can continue to execute and still trigger a pagefault if it
 *   accesses memory addresses < stack_low. This also guarantees that
 *   the exception stack and userspace stack regions do not overlap.
 *
 *   TODO what about multithreaded stacks? The above assumes single thread.
 *
 *   @param stack_high Highest virtual address of the initial stack
 *   @param stack_low Lowest virtual address of the initial stack
 *   @return Void.
 */
void
install_autostack(void *stack_high, void *stack_low)
{
	lprintf("install_autostack() stack_high: %p, stack_low: %p\n",
	        stack_high, stack_low);

	reg_pf_swexn_handler(stack_low);
	return;
}

void
reg_pf_swexn_handler(void *stack_low)
{
	/* Allocate page size for exception stack */
	void *new_stack_high = stack_low + PAGE_SIZE;
	int res = new_pages(new_stack_high, PAGE_SIZE);

	/* Just starting execution and we are out of space */
	//TODO how to check individual error codes?
	if (res < 0)
		lprintf("reg_pf_swexn_handler() failed call to new_pages!\n");

	/* make space for ureg_t */
	ureg_t *newureg = (ureg_t *) new_stack_high;

	/* The parameter esp3 specifies an exception stack; it points to an address
	 * one word higher than the first address that the kernel should use to push
	 * values onto the exception stack
	 */
	void *exn_stack_high = new_stack_high - (sizeof(ureg_t)) + WORD_SIZE;

	void *esp3 = exn_stack_high;

	res = swexn(esp3, pf_swexn_handler, NULL, newureg);
	if (res < 0)
		lprintf("reg_pf_swexn_handler() failed call to swexn()!\n");

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

	/* Only deal with pagefault exceptions */
	if (cause != SWEXN_CAUSE_PAGEFAULT) return;

	/* Only deal with faults caused by non-present page */
	if (!(ERR_P_PROTECTION_VIOLATION_MASK & error_code)) {
		new_pages((void *) (uint32_t) cr2, PAGE_SIZE);
	}
	/* register this handler again */
	reg_pf_swexn_handler((void *) (uint32_t) (cr2 + PAGE_SIZE));
	return;
}

