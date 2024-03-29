/** @file autostack.c
 *
 *  @brief Contains functions for automatic growth of root thread stack
 *         (legacy thread) and pagefault exception handlers as well.
 *
 *  Root thread stack autogrowth:
 *
 *  Pagefault handlers:
 *
 *  When installing pagefault exception handlers with swexn(), the spec
 *  says that argument esp3 "points to an address one word (16 bit == 2 bytes)
 *  higher than the first address that the kernel should use to push values
 *  onto the exception stack". Let exn_stack be the lowest address in use
 *  for any exception handler stack. exn_stack + PAGE_SIZE - 1 be the highest
 *  address in use. However, we have 4 byte alignment for stacks and so the
 *  first address that the kernel should use to push values onto the
 *  exception stack should be exn_stack + PAGE_SIZE - 4. Thus an address
 *  one word higher than exn_stack + PAGE_SIZE - 4 is exactly
 *  exn_stack + PAGE_SIZE - 2 == exn_stack + PAGE_SIZE - WORD_SIZE
 *
 *  Since the thread handler is "not expected to "correct" or otherwise resolve
 *  any exceptions other than pagefaults, if the handler receives a fault
 *  other than pagefaults, we do not resolve the fault, but invoke panic with
 *  a helpful error message, because the spec does say "What should happen if
 *  a thread is killed by an exception? ... be a good idea to think about
 *  whether there is anything reasonable you could do with "a bit" of code and,
 *  if so, to try to do it."
 *
 *  @author Nicklaus Choo (nchoo)
 *  @bug No known bugs.
 */

#include <simics.h>
#include <ureg.h> /* ureg_t */
#include <syscall.h> /* PAGE_SIZE */
#include <stdint.h>  /* uint32_t */
#include <assert.h> /* assert() */
#include <thr_internals.h> /* THR_INITIALIZED */
#include <simics.h> /* MAGIC_BREAK */
#include <autostack_internals.h> /* Fuction prototypes of this file */

/* Number of bytes in a word (16-bit according to ia32-summary.pdf page 2) */
#define WORD_SIZE 2

/* Lowest bit P=0 if fault by non-present page, 1 o/w (intel-sys.pdf pg 182) */
#define PERMISSION_ERR 1

volatile void *global_stack_low = 0;

thr_status_t root_tstatus;

/* Private alternate "stack space" for page fault exception handling */
static char exn_stack[PAGE_SIZE];

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
	assert(global_stack_low == 0);
	global_stack_low = stack_low;

	/* Initialize thread status for root thread except for exit_cvar */
	root_tstatus.thr_stack_low = stack_low;
	root_tstatus.thr_stack_high = stack_high;
	root_tstatus.tid = gettid();
	root_tstatus.exited = 0;
	root_tstatus.status = 0;

	/* esp3 argument points to an address 1 word higher than first address */
	Swexn(exn_stack + PAGE_SIZE - WORD_SIZE, pf_swexn_handler, 0, 0);

	return;
}

/** @ brief Wrapper that deals with error return values for swexn() syscall.
 *
 *  Given that newureg is either 0 or a ureg that was obtained from a
 *  pagefault exception, if the kernel is implemented correctly then installing
 *  the handler must never fail. Thus we invoke panic() if swexn() returns a
 *  negative value. Furthermore, we dont want the handler installation to fail
 *  silently as programs that expect an automatically growing stack would
 *  potentially receive unexpected pagefaults e.g. not due to permission errors.
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
		panic("Syscall swexn() failed installing pagefault handler, error:%d\n",
		      res);
	}
	return;
}
/** @brief Child threads are not supposed to use more than their allocated
 *         stack space and thus if this handler is ever invoked we invoke
 *         panic().
 *  @param arg Argument which should always be 0 for user thread library.
 *  @param ureg Struct containing information on page fault.
 *  @return Void.
 */
void child_pf_handler( void *arg, ureg_t *ureg )
{
	assert(arg == 0);
	affirm_msg(ureg, "Supplied ureg cannot be NULL");

    /* Get relevant info */
	unsigned int cause = ureg->cause;
	unsigned int cr2 = ureg->cr2;
	unsigned int error_code = ureg->error_code;

	/* Do not grow stack space of child */
	if (cause == SWEXN_CAUSE_PAGEFAULT
		&& !(PERMISSION_ERR & error_code)) {
		panic("Pagefaulted at address: 0x%x, disallow allocating more memory "
		      "to child thread stack", cr2);

	/* Do not resolve any other exceptions, just call panic with some info */
	} else {
        panic("Non-Pagefault software exception encountered, cause: 0x%x, "
		      "cr2: 0x%x, error_code: 0x%x", cause, cr2, error_code);
	}
}

void
install_child_pf_handler( void *child_thr_stack_high )
{
	Swexn(child_thr_stack_high + PAGE_SIZE - WORD_SIZE, child_pf_handler, 0, 0);
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

	/* If other user threads initialized, don't grow stack */
	if (THR_INITIALIZED) return;

	/* Get relevant info */
	unsigned int cause = ureg->cause;
	unsigned int cr2 = ureg->cr2;
	unsigned int error_code = ureg->error_code;

	/* deal with non-present pagefaults within PAGE_SIZE away from stack top */
	if (cause == SWEXN_CAUSE_PAGEFAULT
		&& !(PERMISSION_ERR & error_code)
		&& cr2 >= ((unsigned int) global_stack_low) - PAGE_SIZE) {

		/* Allocate new memory for user space stack */
		uint32_t base = ((cr2 / PAGE_SIZE) * PAGE_SIZE);
		int res = new_pages((void *) base, PAGE_SIZE);
		global_stack_low = (void *) base;
		root_tstatus.thr_stack_low = (void *) base;

		/* Panic if cannot grow user space stack for root thread */
		if (res < 0)
			panic("Unable to grow user space stack, error: %d\n", res);

		/* Always register page fault exception handler again */
		Swexn(exn_stack + PAGE_SIZE - WORD_SIZE, pf_swexn_handler, 0, ureg);

	/* Do not resolve any other exceptions, just call panic with some info */
	} else {
		if (cause == SWEXN_CAUSE_PAGEFAULT) {
			if (cr2 < ((unsigned int) global_stack_low) - PAGE_SIZE) {
				panic("Root thread pagefaulted at address 0x%x that was too "
				      "far from lowest stack address", cr2);
			}
			if (PERMISSION_ERR & error_code) {
				panic("Root thread pagefaulted at address 0x%x that "
				      "caused permission access error", cr2);
			}
		} else {
            panic("Non-Pagefault software exception encountered, "
			      "cause: 0x%x, cr2: 0x%x, error_code: 0x%x", cause, cr2,
				  error_code);
		}
	}
	return;
}

