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
	/* Type cast will not result in undefined behavior since 32-bit pointers */
	assert(((uint32_t) stack_low) % PAGE_SIZE == 0);

	/* stack_high - stack_low should always be a multiple of PAGE_SIZE */

    /* if region is not PAGE_SIZE aligned, create a new region that is
	 * PAGE_SIZE aligned
	 */
	lprintf("install_autostack() given stack_high: %p, stack_low: %p\n",
	        stack_high, stack_low);

	//int res;
	//if ((stack_high - stack_low) % PAGE_SIZE != 0) {
	//	res = remove_pages(stack_high);
	//	if (res < 0)
	//		lprintf("install_autostack() failed call to remove_pages: %d\n", res);
	//	uint32_t num_pages =
	//	  (((uint32_t) (long unsigned int) (stack_high - stack_low)) /
	//	   PAGE_SIZE) + 1;
	//	res = new_pages(stack_high, num_pages * PAGE_SIZE);
	//	if (res < 0)
	//		lprintf("install_autostack() failed call to new_pages: %d\n", res);

	//    stack_low = stack_high + (num_pages * PAGE_SIZE);
	//}
	//assert((stack_high - stack_low) % PAGE_SIZE == 0);

	//lprintf("install_autostack() adjusted stack_high: %p, stack_low: %p\n",
	//        stack_high, stack_low);

	reg_pf_swexn_handler(stack_low, NULL);
	return;
}

void
init_newureg(ureg_t *newureg)
{
	assert(newureg);
	newureg->cause = 0;
	newureg->cr2 = 0;
	newureg->ds = 0;
	newureg->es = 0;
	newureg->fs = 0;
	newureg->gs = 0;
	newureg->edi = 0;
	newureg->esi = 0;
	newureg->ebp = 0;
	newureg->zero = 0;
	newureg->ebx = 0;
	newureg->edx = 0;
	newureg->ecx = 0;
	newureg->eax = 0;
	newureg->error_code = 0;
	newureg->eip = 0;
	newureg->cs = 0;
	newureg->eflags = 0;
	newureg->esp = 0;
	newureg->ss = 0;
}

/** @brief Helper for registering a new page fault software exception
 *         handler
 *
 *  If stack_low is non-NULL, we want to create a new region in memory for
 *  the exception handler. This means that oldureg must be set to NULL.
 *  If stack_low is NULL, we want to reuse the old exception handler stack.
 *  This means that oldureg must not be NULL
 *
 *  @param stack_low Lowest address of user space stack
 *  @param oldureg pointer to a ureg iff stack_low == NULL
 *  @return Void.
 */
void
reg_pf_swexn_handler(void *stack_low, ureg_t *oldureg)
{
	ureg_t *newureg = NULL;

	if (stack_low) {
		assert(oldureg == NULL);

		/* Allocate page size for exception stack PAGE_SIZE away from
		 * user stack
		 */
		//TODO what if out of stack space when running pf_swexn_handler ?
		void *new_stack_high = stack_low + PAGE_SIZE;
		int res = new_pages(new_stack_high, PAGE_SIZE);

		/* Just starting execution and we are out of space */
		//TODO how to check individual error codes?
		if (res < 0)
			lprintf("reg_pf_swexn_handler() failed call to new_pages!: %d\n", res);

		newureg = (ureg_t *) new_stack_high;
	} else {
		assert(stack_low == NULL);
		newureg = oldureg;
	}
	/* Zero all struct fields */
	init_newureg(newureg);

	/* The parameter esp3 specifies an exception stack; it points to an address
	 * one word higher than the first address that the kernel should use to
	 * push values onto the exception stack
	 */
	void *exn_stack_high = newureg - (sizeof(ureg_t));
	void *esp3 = exn_stack_high + WORD_SIZE;

	int res = swexn(esp3, pf_swexn_handler, NULL, newureg);
	if (res < 0)
		lprintf("reg_pf_swexn_handler() failed call to swexn(): %d\n", res);
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

		/* Allocate new memory for user space stack */
		new_pages((void *) (uint32_t) cr2, PAGE_SIZE);

		/* Remove page for exception stack, register this handler again */
		int res = remove_pages((void *) (uint32_t) (cr2 + PAGE_SIZE));
		if (res < 0) {
			lprintf("pf_swexn_hander() call to remove_pages() failed: %d\n",
			         res);
		}
		reg_pf_swexn_handler((void *) (uint32_t) (cr2 + 2 * PAGE_SIZE), NULL);

	/* Reuse old exception stack */
	} else {
		reg_pf_swexn_handler(NULL, ureg);
	}
	return;
}

