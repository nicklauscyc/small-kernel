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
	//assert(is_valid_pd((void *)TABLE_ADDRESS(get_cr3())));

	/* Acknowledge interrupt immediately */
    outb(INT_CTL_PORT, INT_ACK_CURRENT);
	//assert(is_valid_pd((void *)TABLE_ADDRESS(get_cr3())));


	/* Only allow forking of task that has 1 thread */
	tcb_t *parent_tcb = get_running_thread();
	//assert(is_valid_pd((void *)TABLE_ADDRESS(get_cr3())));

	assert(parent_tcb);
	int num_threads = get_num_threads_in_owning_task(parent_tcb);
	log_info("fork(): "
			 "Forking task with number of threads:%ld", num_threads);

	if (num_threads > 1) {
		return -1;
	}
	//assert(is_valid_pd((void *)TABLE_ADDRESS(get_cr3())));

	assert(num_threads == 1);
	//affirm(is_valid_pd((void *)TABLE_ADDRESS(get_cr3())));

	/* Get parent_pd in kernel memory, unaffected by paging */
	uint32_t cr3 = get_cr3();
	//affirm(is_valid_pd((void *)TABLE_ADDRESS(get_cr3())));

	uint32_t *parent_pd = (uint32_t *) (cr3 & ~(PAGE_SIZE - 1));
	assert((uint32_t) parent_pd < USER_MEM_START);
	//affirm(is_valid_pd(parent_pd));


	/* Create child_pd as a deep copy */
	uint32_t *child_pd = new_pd_from_parent((void *)parent_pd);
	//affirm(is_valid_pd(parent_pd));
	//affirm(is_valid_pd(child_pd));

	//assert(PAGE_ALIGNED(child_pd));
	log_info("fork(): "
			 "new child_pd at address:%p", child_pd);

	/* Create child pcb and tcb */
	uint32_t child_pid, child_tid;
	if (create_pcb(&child_pid, child_pd) < 0) {
		// TODO: delete page directory
		return -1;
	}
	//affirm(is_valid_pd(parent_pd));
	//affirm(is_valid_pd(child_pd));


	if (create_tcb(child_pid, &child_tid) < 0) {
		// TODO: delete page directory
		// TODO: delete_pcb of parent
		return -1;
	}
	//affirm(is_valid_pd(parent_pd));
	//affirm(is_valid_pd(child_pd));

	tcb_t *child_tcb;
	assert(child_tcb = find_tcb(child_tid));

//#ifndef NDEBUG
//    /* Register this task with simics for better debugging */
//    sim_reg_child(child_pd, (void *) get_cr3());
//#endif
	//affirm(is_valid_pd(parent_pd));
	//affirm(is_valid_pd(child_pd));


	uint32_t *child_kernel_esp_on_ctx_switch;
	uint32_t *parent_kern_stack_hi = get_kern_stack_hi(parent_tcb);
	uint32_t *child_kern_stack_hi = get_kern_stack_hi(child_tcb);

	child_kernel_esp_on_ctx_switch = save_child_regs(parent_kern_stack_hi,
	                                                 child_kern_stack_hi,
													 child_pd);
	//affirm(is_valid_pd(parent_pd));
	//affirm(is_valid_pd(child_pd));

	/* Set child's kernel esp */
	//affirm(child_kernel_esp_on_ctx_switch);
	set_kern_esp(child_tcb, child_kernel_esp_on_ctx_switch);
	//affirm(is_valid_pd(parent_pd));
	//affirm(is_valid_pd(child_pd));

	/* If logging is set to debug, this will print stuff */
	log_print_parent_and_child_stacks(parent_tcb, child_tcb );
	//affirm(is_valid_pd(parent_pd));
	//affirm(is_valid_pd(child_pd));

    /* After setting up child stack and VM, register with scheduler */
    if (make_thread_runnable(get_tcb_tid(child_tcb)) < 0)
        return -1;
	//affirm(is_valid_pd(parent_pd));
	//affirm(is_valid_pd(child_pd));

    /* Only parent will return here */
    assert(get_running_tid() == get_tcb_tid(parent_tcb));
	//affirm(is_valid_pd(parent_pd));
	//affirm(is_valid_pd(child_pd));

	return get_tcb_tid(child_tcb);
}




