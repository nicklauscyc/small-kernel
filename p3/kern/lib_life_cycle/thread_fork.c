/** @file thread_fork.c
 *  @brief Contains thread fork interrupt handler
 */

#include <x86/cr.h> /* get_cr3() */
#include <asm.h>				/* outb() */
#include <stdint.h>				/* uint32_t */
#include <logger.h>				/* log_info */
#include <scheduler.h>			/* get_running_thread */
#include <task_manager.h>		/* get_tcb_tid() */
#include <interrupt_defines.h>	/* INT_CTL_PORT, INT_ACK_CURRENT */
#include <lib_thread_management/mutex.h> /* mutex_t */

#include <task_manager_internal.h>

/** @brief Forks a thread in the current running task
 *
 *  @return tid of new thread to parent thread, 0 to child thread.
 */
int
thread_fork( void )
{
	/* Acknowledge interrupt immediately */
    outb(INT_CTL_PORT, INT_ACK_CURRENT);

	tcb_t *parent_tcb = get_running_thread();
	pcb_t *pcb = parent_tcb->owning_task;
	uint32_t child_tid;


	tcb_t *child_tcb = create_tcb(pcb, &child_tid);
	if (!child_tcb) {
		return -1;
	}

	assert(child_tcb == find_tcb(child_tid));

	/* Set up childrens stack. Child should return to user mode with same
	 * registers as parent. Only %eax will be different. */
	uint32_t *parent_kern_stack_hi = get_kern_stack_hi(parent_tcb);
	uint32_t *child_kern_stack_hi = get_kern_stack_hi(child_tcb);

    /* Set up the child's stack */
	uint32_t *c_esp = child_kern_stack_hi;
	uint32_t *p_esp = parent_kern_stack_hi;

	/* Pushed onto stack by processor on mode switch */
	*(--c_esp) = *(--p_esp); /* SS */
	*(--c_esp) = *(--p_esp); /* esp*/
	*(--c_esp) = *(--p_esp); /* e_flags*/
	*(--c_esp) = *(--p_esp); /* cs */
	*(--c_esp) = *(--p_esp); /* eip */

	/* Pushed onto stack by pusha in call_thread_fork */
	*(--c_esp) = 0; /* eax */
	--p_esp;
	*(--c_esp) = *(--p_esp); /* ecx */
	*(--c_esp) = *(--p_esp); /* edx */
	*(--c_esp) = *(--p_esp); /* ebx */
	*(--c_esp) = *(--p_esp); /* original_esp before pusha */
	*(--c_esp) = *(--p_esp); /* ebp */
	*(--c_esp) = *(--p_esp); /* esi */
	*(--c_esp) = *(--p_esp); /* edi */

	/* Pushed onto stack by other instructions in call_thread_fork */
	*(--c_esp) = *(--p_esp); /* ds */
	*(--c_esp) = *(--p_esp); /* es */
	*(--c_esp) = *(--p_esp); /* fs */
	*(--c_esp) = *(--p_esp); /* gs */
	*(--c_esp) = *(--p_esp); /* return address to call_thread_fork */

	/* For context switch */
	*(--c_esp) = 0; /* dummy ebp */
	*(--c_esp) = 0; /* dummy eax */
	*(--c_esp) = 0; /* dummy ebx */
	*(--c_esp) = 0; /* dummy ecx */
	*(--c_esp) = 0; /* dummy edx */
	*(--c_esp) = 0; /* dummy edi */
	*(--c_esp) = 0; /* dummy esi */

	uint32_t cr0 = get_cr0();
	*(--c_esp) = cr0; /* cr0 */

	uint32_t cr3 = get_cr3();
	uint32_t parent_pd = (cr3 & ~(PAGE_SIZE - 1));
	*(--c_esp) = parent_pd; /* cr3 */

	/* Set child's kernel esp */
	affirm(STACK_ALIGNED(c_esp));
	affirm(c_esp);
	set_kern_esp(child_tcb, c_esp);

    /* After setting up child stack and VM, register with scheduler */
    if (make_thread_runnable(child_tcb) < 0) {
		log_warn("thread_fork(): unable to make child thread runnable");
        return -1;
	}

    /* Only parent will return here */
    assert(get_running_tid() == get_tcb_tid(parent_tcb));

	return get_tcb_tid(child_tcb);
}
