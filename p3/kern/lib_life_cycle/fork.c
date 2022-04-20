/** @file fork.c
 *  @brief Contains fork interrupt handler and helper functions for
 *  	   installation
 *
 *
 */

#include <logger.h> /* log() */
#include <assert.h>
#include <x86/interrupt_defines.h> /* INT_CTL_PORT, INT_ACK_CURRENT */
#include <x86/cr.h> /* get_cr3() */
#include <common_kern.h> /* USER_MEM_START */
#include <malloc.h> /* smemalign() */
#include <string.h> /* memcpy() */
#include <page.h> /* PAGE_SIZE */
#include <task_manager.h>
#include <task_manager_internal.h>

#include <memory_manager.h> /* new_pd_from_parent, PAGE_ALIGNED() */
#include <simics.h>
#include <scheduler.h>
#include <x86/asm.h> /* outb() */

// saves regs and returns new esp
void *save_child_regs(void *parent_kern_esp, void *child_kern_esp,
					  void *child_cr3 );

/** @brief Prints the parent and child stacks on call to fork()
 *
 *  @parent_tcb Parent's thread control block
 *  @child_tcb Child's thread control block
 *  @return Void.
 */
void
log_print_parent_and_child_stacks( tcb_t *parent_tcb, tcb_t *child_tcb )
{
	log("print parent stack");
	for (int i = 0; i < 32; ++i) {
		log("address:%p, value:0x%lx",
		(uint32_t *) get_kern_stack_hi(parent_tcb) - i,
		*((uint32_t *) get_kern_stack_hi(parent_tcb) - i));
	}
	log("print child stack");
	for (int i = 0; i < 32; ++i) {
		log("address:%p, value:0x%lx",
		(uint32_t *) get_kern_stack_hi(child_tcb) - i,
		*((uint32_t *) get_kern_stack_hi(child_tcb) - i));
	}
	log("result from get_running_tid():%d", get_running_tid());
}

/** @brief Fork task into two.
 *
 *  @return 0 for child thread, child tid for parent thread, -1 on error  */
int
fork( void )
{
	/* Acknowledge interrupt immediately */
    outb(INT_CTL_PORT, INT_ACK_CURRENT);


	/* Only allow forking of task that has 1 thread */
	tcb_t *parent_tcb = get_running_thread();
	affirm(parent_tcb);
	pcb_t *parent_pcb = get_running_task();
	affirm(parent_pcb);


	int num_threads = get_num_active_threads_in_owning_task(parent_tcb);
	log_warn("fork(): "
		 "Forking task with number of threads:%ld", num_threads);

	if (num_threads > 1) {
		MAGIC_BREAK;
		return -1;
	}
	assert(num_threads == 1);

	/* Get parent_pd in kernel memory, unaffected by paging */
	uint32_t cr3 = get_cr3();

	uint32_t *parent_pd = (uint32_t *) (cr3 & ~(PAGE_SIZE - 1));
	assert((uint32_t) parent_pd < USER_MEM_START);


	/* Create child_pd as a deep copy */
	uint32_t *child_pd = new_pd_from_parent((void *)parent_pd);

	//assert(PAGE_ALIGNED(child_pd));
	log_info("fork(): "
			 "new child_pd at address:%p", child_pd);

	/* Create child pcb and tcb */
	uint32_t child_pid, child_tid;
	if (create_pcb(&child_pid, child_pd, parent_pcb) < 0) {
		// TODO: delete page directory
		MAGIC_BREAK;
		return -1;
	}


	if (create_tcb(child_pid, &child_tid) < 0) {
		// TODO: delete page directory
		// TODO: delete_pcb of parent
		MAGIC_BREAK;
		return -1;
	}

	tcb_t *child_tcb;
	assert(child_tcb = find_tcb(child_tid));
	pcb_t *child_pcb = child_tcb->owning_task;


	/* Register this task with simics for better debugging */
#ifndef NDEBUG
        sim_reg_child(child_pd, parent_pd);
#endif

		uint32_t *child_kernel_esp_on_ctx_switch;
	uint32_t *parent_kern_stack_hi = get_kern_stack_hi(parent_tcb);
	uint32_t *child_kern_stack_hi = get_kern_stack_hi(child_tcb);

	child_kernel_esp_on_ctx_switch = save_child_regs(parent_kern_stack_hi,
	                                                 child_kern_stack_hi,
													 child_pd);
	/* Set child's kernel esp */
	affirm(child_kernel_esp_on_ctx_switch);
	set_kern_esp(child_tcb, child_kernel_esp_on_ctx_switch);

	/* If logging is set to debug, this will print stuff */
	log_print_parent_and_child_stacks(parent_tcb, child_tcb );

	/* No need for locking as only 1 active thread */
	Q_INSERT_TAIL(&(parent_pcb->active_child_tasks_list), child_pcb,
			              vanished_child_tasks_link);
	parent_pcb->num_active_child_tasks++;

    /* After setting up child stack and VM, register with scheduler */
    if (make_thread_runnable(get_tcb_tid(child_tcb)) < 0) {
		MAGIC_BREAK;
        return -1;
	}
    /* Only parent will return here */
    assert(get_running_tid() == get_tcb_tid(parent_tcb));

	return get_tcb_tid(child_tcb);
}




