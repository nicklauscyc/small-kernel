/** @file thread_fork.c
 *  @brief Contains thread fork interrupt handler
 */

#include <asm.h>				/* outb() */
#include <stdint.h>				/* uint32_t */
#include <logger.h>				/* log_info */
#include <scheduler.h>			/* get_running_thread */
#include <task_manager.h>		/* get_tcb_tid() */
#include <interrupt_defines.h>	/* INT_CTL_PORT, INT_ACK_CURRENT */
#include <lib_thread_management/mutex.h> /* mutex_t */

#include <task_manager_internal.h>

// saves regs and returns new esp
void *save_child_regs(void *parent_kern_esp, void *child_kern_esp,
					  void *child_cr3 );
int
thread_fork( void )
{
	/* Acknowledge interrupt immediately */
    outb(INT_CTL_PORT, INT_ACK_CURRENT);

	tcb_t *parent_tcb = get_running_thread();
	pcb_t *pcb = parent_tcb->owning_task;
	uint32_t child_tid;

	if (create_tcb(pcb, &child_tid) < 0) {
		return -1;
	}

	tcb_t *child_tcb;
	assert(child_tcb = find_tcb(child_tid));

	/* Set up childrens stack. Child should return to user mode with same
	 * registers as parent. Only %eax will be different. */
	uint32_t *child_kernel_esp_on_ctx_switch;
	uint32_t *parent_kern_stack_hi = get_kern_stack_hi(parent_tcb);
	uint32_t *child_kern_stack_hi = get_kern_stack_hi(child_tcb);

	child_kernel_esp_on_ctx_switch = save_child_regs(parent_kern_stack_hi,
	                                                 child_kern_stack_hi,
													 pcb->pd);
	/* Set child's kernel esp */
	affirm(child_kernel_esp_on_ctx_switch);
	set_kern_esp(child_tcb, child_kernel_esp_on_ctx_switch);

	/* TODO: REMOVE? If logging is set to debug, this will print stuff */
	//log_print_parent_and_child_stacks(parent_tcb, child_tcb );

	// FIXME: Maybe this should be inside create_tcb
	// FIXED: dw this is for adding tasks, we have a new thread and
	// create_tcb automatically does this
	//mutex_lock(&(pcb->set_status_vanish_wait_mux));
	//Q_INSERT_TAIL(&(pcb->active_child_tasks_list), pcb,
	//		              vanished_child_tasks_link);
	//pcb->num_active_child_tasks++;
	//mutex_unlock(&(pcb->set_status_vanish_wait_mux));

    /* After setting up child stack and VM, register with scheduler */
    if (make_thread_runnable(child_tcb) < 0) {
		log_info("fork(): unable to make child thread runnable");
        return -1;
	}

    /* Only parent will return here */
    assert(get_running_tid() == get_tcb_tid(parent_tcb));

	return get_tcb_tid(child_tcb);
}
