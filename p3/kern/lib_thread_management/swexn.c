/** @file swexn.c
 *  @brief Swexn syscall and facilities for software exception handling */

#include <seg.h>			/* SEGSEL_USER_CS */
#include <ureg.h>			/* ureg_t */
#include <stdint.h>			/* uint32_t  */
#include <syscall.h>		/* swexn_handler_t */
#include <scheduler.h>		/* get_running_thread */
#include <iret_travel.h>	/* iret_travel */
#include <task_manager.h>	/* tcb_t */
#include <atomic_utils.h>	/* compare_and_swap_atomic */

/* FIXME: Proper abstraction? */
#include <task_manager_internal.h>

/** @brief Sets registers according to newureg. Assumes
 *		   newureg is valid (non-NULL and won't crash iret)
 *		   and travels back to user mode with these registers.
 *
 *	Defined in kern/lib_thread_management/swexn_set_regs.S */
void
swexn_set_regs( ureg_t *newureg );

static int
valid_handler( void *esp3, swexn_handler_t eip )
{
	if (!esp3 && !eip)
		return 1;

	if ((!esp3 && eip) || (esp3 && !eip))
		return 0;

	/* TODO: If want to register, ensure validity of pointers */


	return 1;
}

static int
valid_newureg( ureg_t *newureg )
{
	if (!newureg)
		return 1;

	/* TODO: Check for valid register values. Namely, register
	 * values for cs, eip, eflags, ss and eip should be legal
	 * for iret invocation. */

	return 1;
}

static int
cause_has_error_code( unsigned int cause )
{
	return cause == SWEXN_CAUSE_SEGFAULT || cause == SWEXN_CAUSE_STACKFAULT ||
			cause == SWEXN_CAUSE_PROTFAULT || cause == SWEXN_CAUSE_PAGEFAULT ||
			cause == SWEXN_CAUSE_ALIGNFAULT;
}

static void
fill_ureg( ureg_t *ureg, uint32_t *ebp, unsigned int cause, unsigned int cr2 )
{
	ureg->cause = cause;

	if (cause == SWEXN_CAUSE_PAGEFAULT)
		ureg->cr2 = cr2;
	else
		ureg->cr2 = 0;

	ureg->ds = SEGSEL_USER_DS;
	ureg->es = SEGSEL_USER_DS;
	ureg->fs = SEGSEL_USER_DS;
	ureg->gs = SEGSEL_USER_DS;

	/* Fault handler asm wrapper does a pusha on entry */
	uint32_t *ebp_temp = ebp;
	ureg->eax = *(--ebp_temp);
	ureg->ecx = *(--ebp_temp);
	ureg->edx = *(--ebp_temp);
	ureg->ebx = *(--ebp_temp);
	ureg->zero = 0; --ebp_temp; // temp/esp
	ureg->ebp = *(--ebp_temp);
	ureg->esi = *(--ebp_temp);
	ureg->edi = *(--ebp_temp);

	if (cause_has_error_code(cause))
		ureg->error_code = *(++ebp);
	else
		ureg->error_code = 0;

	ureg->eip = *(++ebp);
	ureg->cs = *(++ebp);
	ureg->eflags = *(++ebp);

	/* Since this handler is only for exceptions caused in user mode, esp
	 * and ss must be on the stack. */
	ureg->esp = *(++ebp);
	ureg->ss = *(++ebp);
}

/** @brief Try to call software exception handler. Iff it is installed,
 *		   this function never returns. Will only attempt to handle fault
 *		   if it is a user caused software exception.
 *
 *	@arg ebp	Ebp. Points to one word lower than error code/eip in
 *				kernel stack after a fault takes place.
 *	@arg cause	Cause of fault.
 *	@arg cr2	If this is called because of a pagefault, cr2 should
 *				be the vm address which triggered the fault. */
void
handle_exn( uint32_t *ebp, unsigned int cause, unsigned int cr2 )
{
	uint32_t cs, eflags;

	if (cause_has_error_code(cause)) {
		cs = *(ebp + 3);
		eflags = *(ebp + 4);
	} else {
		cs = *(ebp + 2);
		eflags = *(ebp + 3);
	}

	if (cs != SEGSEL_USER_CS) /* Only handle user exceptions */
		return;

	tcb_t *tcb = get_running_thread();
	/* Avoid concurrency issues among different interrupts */
	if (compare_and_swap_atomic((uint32_t *)&(tcb->has_swexn_handler), 1, 0)) {
		/* Set up handler stack */
		uint32_t *stack_lo = (uint32_t *)tcb->swexn_stack;
		assert(sizeof(ureg_t) % 4 == 0);
		stack_lo -= sizeof(ureg_t)/4;
		fill_ureg((ureg_t *)stack_lo, ebp, cause, cr2);	/* ureg on stack */
		stack_lo--;
		*(stack_lo) = ((uint32_t)stack_lo) + 4;		/* pointer to ureg stack */
		*(--stack_lo) = (uint32_t)tcb->swexn_arg;	/* arg on stack */
		stack_lo--;									/* Bogus return address */

		uint32_t handler = (uint32_t)tcb->swexn_handler;

		/* Deregister this handler */
		tcb->swexn_handler = 0;
		tcb->swexn_stack = 0;

		iret_travel(handler, SEGSEL_USER_CS, eflags,
				(uint32_t)stack_lo, SEGSEL_USER_DS);
	}
}

int
swexn( void *esp3, swexn_handler_t eip, void *arg, ureg_t *newureg )
{
	/* Since arg is only ever used by the user software exception handler,
	 * no validation of it need be done. */
	/* Ensure valid combination of requests */
	if (!valid_handler(esp3, eip) || !valid_newureg(newureg))
		return -1;

	tcb_t *tcb = get_running_thread();

	/* Register/Unregister handler */
	tcb->swexn_handler		= (uint32_t)eip;
	tcb->swexn_stack		= (uint32_t)esp3;
	tcb->swexn_arg			= arg;
	tcb->has_swexn_handler	= !!eip;

	/* Set user registers, if they were provided */
	if (newureg) /* This won't return */
		swexn_set_regs(newureg);

	return 0;
}

