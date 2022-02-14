/** @file autostack.c
 *
 *  @brief Automatically grows the stack for the single-threaded environment
 *         set up before main().
 *
  *   We allocate memory at address stack_low + PAGE_SIZE so that userspace
 *   execution can continue to execute and still trigger a pagefault if it
 *   accesses memory addresses < stack_low. This also guarantees that
 *   the exception stack and userspace stack regions do not overlap.
 *
 *   TODO what about multithreaded stacks? The above assumes single thread.

 *
 *  @author Nicklaus Choo (nchoo)
 *  @bug No known bugs.
 */

#include <ureg.h> /* ureg_t */
#include <syscall.h> /* PAGE_SIZE */
#include <simics.h> /* lprintf() */
#include <stdint.h>  /* uint32_t */
#include <assert.h> /* assert() */

/* Lowest bit P=0 if fault by non-present page, 1 o/w (intel-sys.pdf pg 182) */
#define ERR_P_PROTECTION_VIOLATION_MASK 1

/* Word size is 16 bit so 2 bytes */
#define WORD_SIZE 2

/* Private alternate "stack space" for page fault exception handling */
static char exn_stack[PAGE_SIZE];

void install_autostack(void * stack_high, void * stack_low);
void pf_swexn_handler(void *arg, ureg_t *ureg);

/** @ brief Wrapper that deals with error return values for swexn() syscall.
 *
 *  Given that newureg is either 0 or a ureg that was obtained from a
 *  pagefault exception, if the kernel is implemented correctly then installing
 *  the handler must never fail. Thus we invoke panic() if swexn() returns a
 *  negative value.
 *
 *  @param esp3 Address 1 word higher than the first address kernel should use.
 *  @param eip Address of the pagefault handler function
 *  @param arg Opaque pointer to arguments needed for pagefault handler. For
 *         the user thread library, this is always 0
 *  @param newureg Struct that tells kernel what register values to use. For
 *         the user thread library, 0 if installing handler the first time, and
 *         the ureg supplied to the handler when it was called for
 *         reinstallations.
 */
void
Swexn(void *esp3, swexn_handler_t eip, void *arg, ureg_t *newureg)
{
	assert(!arg);

	int res = 0;
	if ((res = swexn(esp3, eip, arg, newureg)) < 0) {
		panic("FATAL: swexn() failed installing pagefault handler, error:%d\n",
		      res);
	}
	return;
}

/** @brief Installs pagefault handler at a region of memory that will never
 *         be touched by main() and all other function calls descending from
 *         main().
 *
 *  The handler is installed on a "stack" exn_stack. exn_stack is a private
 *  global char array of PAGE_SIZE bytes and is thus in a region of
 *  virtual memory that will not overlap with the execution stack which main()
 *  is on.
 *
 *  @param stack_high Highest virtual address of the initial stack
 *  @param stack_low Lowest virtual address of the initial stack
 *  @return Void.
 */
void
install_autostack(void *stack_high, void *stack_low)
{
	lprintf("stack_high: %p, stack_low: %p size:%x\n", stack_high,
	stack_low, stack_high - stack_low);

	/* esp3 argument points to an address 1 word higher than first address */
	Swexn(exn_stack + PAGE_SIZE + 1, pf_swexn_handler, 0, 0);

	return;
}

/** @brief Page fault exception handler.
 *
 *  @param arg Argument which should always be 0 for user thread library.
 *  @param ureg Struct containing information on page fault.
 *  @return Void.
 */
void pf_swexn_handler(void *arg, ureg_t *ureg)
{
	assert(arg == 0);
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

		/* Allocate new memory for user space stack */
		uint32_t base = ((cr2 / PAGE_SIZE) * PAGE_SIZE);
		int res = new_pages((void *) base, PAGE_SIZE);

		/* Panic if cannot grow user space stack for initial single thread */
		if (res < 0)
			panic("FATAL: Unable to grow user space stack, error: %d\n", res);
	}
	/* Always register page fault exception handler again */
	Swexn(exn_stack + PAGE_SIZE + 1, pf_swexn_handler, 0, ureg);
	return;
}

