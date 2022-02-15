/* If you want to use assembly language instead of C,
 * delete this autostack.c and provide an autostack.S
 * instead.
 */

#include <ureg.h> /* ureg_t */
#include <syscall.h> /* PAGE_SIZE */
#include <simics.h> /* lprintf() */
#include <stdint.h>  /* uint32_t */
#include <stddef.h> /* NULL */
#include <assert.h> /* assert() */

/* intel-sys.pdf pg182 lowest bit P=0 if fault by non-present page, 1 o/w */
#define ERR_P_PROTECTION_VIOLATION_MASK 0x00000001

/* Word size is 16 bit so 2 bytes */
#define WORD_SIZE 2

extern unsigned int the_stack_low;

/* Private alternate "stack space" for page fault exception handling */
static char exn_stack[PAGE_SIZE];

void install_autostack(void * stack_high, void * stack_low);
void pf_swexn_handler(void *arg, ureg_t *ureg);
void reg_pf_swexn_handler(void *stack_low, ureg_t *oldureg);

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
	assert(the_stack_low == 0);
	the_stack_low = (unsigned int) stack_low;
	lprintf("stack_high: %p, stack_low: %p size:%x\n", stack_high,
	stack_low, stack_high - stack_low);

	int res = swexn(exn_stack + PAGE_SIZE + 1, pf_swexn_handler, 0, 0);
	lprintf("exn_stack: %p, res: %d\n", exn_stack, res);
	assert(res == 0);

	return;
}

/** @brief Page fault exception handler
 */
void pf_swexn_handler(void *arg, ureg_t *ureg)
{
	assert(arg == NULL);
	assert(ureg);

	/* Get relevant info */
	int cause = ureg->cause;

	/* Memory address resulting in fault */
	unsigned int cr2 = ureg->cr2;

	/* Why memory address was inaccessible */
	unsigned int error_code = ureg->error_code;

	lprintf("pf_swexn_handler() cause: %x, cr2: %x, error_code: %x\n",
	        cause, cr2, error_code);

	/* Only deal with pagefault exceptions caused by non-present page */
	if (cause == SWEXN_CAUSE_PAGEFAULT
		&& !(ERR_P_PROTECTION_VIOLATION_MASK & error_code)) {
		//MAGIC_BREAK;

		/* Allocate new memory for user space stack */
		uint32_t base = ((cr2 / PAGE_SIZE) * PAGE_SIZE);
		int res = new_pages((void *) base, PAGE_SIZE);
		lprintf("base: %p\n", (void *) base);
		the_stack_low = base; // TODO delete this after stop debug

		/* Panic if cannot grow user space stack */
		if (res < 0)
			panic("FATAL: Unable to grow user space stack, error: %d\n", res);
	}
	/* Always register page fault exception handler again */
	swexn(exn_stack + PAGE_SIZE + 1, pf_swexn_handler, 0, ureg);
	return;
}

