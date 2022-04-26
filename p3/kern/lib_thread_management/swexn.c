/** @file swexn.c
 *  @brief Swexn syscall and facilities for software exception handling */

#include <seg.h>			/* SEGSEL_USER_CS */
#include <ureg.h>			/* ureg_t */
#include <eflags.h>			/* get_eflags, EFL_* */
#include <stdint.h>			/* uint32_t  */
#include <syscall.h>		/* swexn_handler_t */
#include <scheduler.h>		/* get_running_thread */
#include <iret_travel.h>	/* iret_travel */
#include <task_manager.h>	/* tcb_t */
#include <atomic_utils.h>	/* compare_and_swap_atomic */
#include <memory_manager.h> /* is_valid_user_pointer, READ_ONLY, READ_WRITE */

#include <logger.h>
#include <simics.h>

#define EFLAGS_RESERVED_BITS \
	(EFL_RESV1 | EFL_RESV2 | EFL_RESV3 | EFL_IF | \
	EFL_IOPL_RING3 | EFL_NT | EFL_RESV4 | EFL_VM | \
	EFL_VIF | EFL_VIP | EFL_ID)

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
valid_handler_code_and_stack( void *esp, swexn_handler_t eip )
{
	/* Ensure eip points to allocated, read only region */
	if (!is_valid_user_pointer((void *)eip, READ_ONLY)) {
		log_info("[Swexn] Invalid ureg->eip: %p", (void *)eip);
		return 0;
	}

	char *c_esp = (char *)esp;
	c_esp -= 4; /* esp points to 1 word higher than first writable location */

	/* Ensure stack is writable. Check 1 word lower than input esp.
	 * If the user picked too small a stack, that is their problem. */
	if (!is_valid_user_pointer((void *)c_esp, READ_WRITE)) {
		log_info("[Swexn] Invalid ureg->esp: %p", (void *)c_esp);
		return 0;
	}

	return 1;
}

static int
valid_handler( void *esp3, swexn_handler_t eip )
{
	if (!esp3 && !eip)
		return 1;

	if ((!esp3 && eip) || (esp3 && !eip))
		return 0;

	return valid_handler_code_and_stack(esp3, eip);
}

static int
valid_newureg( ureg_t *newureg )
{
	if (!newureg)
		return 1;

	if (newureg->ds != SEGSEL_USER_DS || newureg->es != SEGSEL_USER_DS ||
		newureg->fs != SEGSEL_USER_DS || newureg->gs != SEGSEL_USER_DS) {
		log_warn("[Swexn] Invalid data segment values");
		return 0;
	}

	/* No need to check gp-registers as those only affect user mode execution */

	/* Check for register values used by iret which could potentially crash
	 * the kernel or create security vulnerabilities (by letting user
	 * applications modify protected values) */
	uint32_t eflags = get_eflags();
	uint32_t new_eflags = newureg->eflags;
	uint32_t violations = (eflags ^ new_eflags) & EFLAGS_RESERVED_BITS;

	return valid_handler_code_and_stack((void *)newureg->esp,
			(swexn_handler_t)newureg->eip)
		&& newureg->ss == SEGSEL_USER_DS && newureg->cs == SEGSEL_USER_CS
		&& !violations;
}

static int
cause_has_error_code( unsigned int cause )
{
	return cause == SWEXN_CAUSE_SEGFAULT || cause == SWEXN_CAUSE_STACKFAULT ||
			cause == SWEXN_CAUSE_PROTFAULT || cause == SWEXN_CAUSE_PAGEFAULT ||
			cause == SWEXN_CAUSE_ALIGNFAULT;
}

static void
fill_ureg( ureg_t *ureg, int *ebp, unsigned int cause, unsigned int cr2 )
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
	int *ebp_temp = ebp;
	ureg->ebp = *ebp;

	ureg->eax = *(--ebp_temp);
	ureg->ecx = *(--ebp_temp);
	ureg->edx = *(--ebp_temp);
	ureg->ebx = *(--ebp_temp);
	ureg->zero = 0; --ebp_temp; // temp/esp
	ureg->esi = *(--ebp_temp);
	ureg->edi = *(--ebp_temp);

	if (cause_has_error_code(cause))
		ureg->error_code = *(++ebp);
	else
		ureg->error_code = 0;

	ureg->eip	 = *(++ebp);
	ureg->cs	 = *(++ebp);
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
handle_exn( int *ebp, unsigned int cause, unsigned int cr2 )
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
	log_warn("Esp3 is %p", esp3);
	MAGIC_BREAK; // this causes a triple fault

	/* Since arg is only ever used by the user software exception handler,
	 * no validation of it need be done. */
	/* Ensure valid combination of requests */
	if (!valid_handler(esp3, eip) || !valid_newureg(newureg))
		return -1;

	tcb_t *tcb = get_running_thread();

	/* Register/Unregister handler */
	log_warn("Swexn called: Setting handler for tcb %p", tcb);
	tcb->swexn_handler		= (uint32_t)eip;
	tcb->swexn_stack		= (uint32_t)esp3;
	tcb->swexn_arg			= arg;
	tcb->has_swexn_handler	= !!eip;

	/* Set user registers, if they were provided */
	if (newureg) /* This won't return */
		swexn_set_regs(newureg);

	return 0;
}

