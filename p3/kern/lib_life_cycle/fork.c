/** @file fork.c
 *  @brief Contains fork interrupt handler and helper functions for
 *  	   installation
 *
 *
 */

#include <logger.h> /* log() */
#include <assert.h>
#include <x86/asm.h> /* idt_base() */
#include <install_handler.h> /* install_handler_in_idt() */
#include <x86/interrupt_defines.h> /* INT_CTL_PORT, INT_ACK_CURRENT */
#include <x86/cr.h> /* get_cr3() */
#include <common_kern.h> /* USER_MEM_START */
#include <malloc.h> /* smemalign() */
#include <string.h> /* memcpy() */
#include <page.h> /* PAGE_SIZE */
#include <task_manager.h> /* TODO this must be deleted after demo, breaks interface */
#include <memory_manager.h> /* new_pd_from_parent, PAGE_ALIGNED() */
#include <simics.h>
#include <scheduler.h>

// saves regs and returns new esp
void *save_child_regs(void *parent_kern_esp, void *child_kern_esp);

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
		log("address:%p, value:0x%lx", parent_tcb->kernel_stack_hi - i,
		*(parent_tcb->kernel_stack_hi - i));
	}
	log("print child stack");
	for (int i = 0; i < 32; ++i) {
		log("address:%p, value:0x%lx", child_tcb->kernel_stack_hi - i,
		*(child_tcb->kernel_stack_hi - i));
	}
	log("result from get_running_tid():%d", get_running_tid());
}

void
init_fork( void )
{
	/* honestly not sure what to init */
}

/** @brief Fork task into two.
 *
 *  @return 0 for child thread, child tid for parent thread, -1 on error  */
int
fork( void )
{
	/* Only allow forking of task that has 1 thread */
	int tid = get_running_tid();
	tcb_t *parent_tcb = find_tcb(tid);
	assert(parent_tcb);
	int num_threads = get_num_threads_in_owning_task(parent_tcb);

	/* Acknowledge interrupt */
    outb(INT_CTL_PORT, INT_ACK_CURRENT);

	/* Only fork if task has 1 thread */
	log("Forking task with number of threads:%ld", num_threads);
	if (num_threads > 1) {
		return -1;
	}
	assert(num_threads == 1);

	/* Get parent_pd in kernel memory, unaffected by paging */
	uint32_t cr3 = get_cr3();
	uint32_t *parent_pd = (uint32_t *) (cr3 & ~(PAGE_SIZE - 1));
	assert((uint32_t) parent_pd < USER_MEM_START);

	/* Create child_pd as a deep copy */
    uint32_t *child_pd = new_pd_from_parent((void *)parent_pd);
	assert(PAGE_ALIGNED(child_pd));
	log("new child_pd at address:%p", child_pd);

	/* Create child pcb and tcb */
    uint32_t child_pid, child_tid;
    if (create_pcb(&child_pid, child_pd) < 0) {
        // TODO: delete page directory
        return -1;
    }
    if (create_tcb(child_pid, &child_tid) < 0) {
        // TODO: delete page directory
        // TODO: delete_pcb of parent
        return -1;
    }
	tcb_t *child_tcb;
    assert(child_tcb = find_tcb(child_tid));

#ifndef NDEBUG
    /* Register this task with simics for better debugging */
	// TODO what is elf->e_fname for this guy?
    //sim_reg_process(pd, elf->e_fname);
#endif

	uint32_t *child_kernel_esp_on_ctx_switch;
	uint32_t *parent_kern_stack_hi = get_kern_stack_hi(parent_tcb);
	uint32_t *child_kern_stack_hi = get_kern_stack_hi(child_tcb);

	child_kernel_esp_on_ctx_switch = save_child_regs(parent_kern_stack_hi,
												     child_kern_stack_hi);
	/* Set child's kernel esp */
	affirm(child_kernel_esp_on_ctx_switch);
	set_kern_esp(child_tcb, child_kernel_esp_on_ctx_switch);

	/* If logging is set to debug, this will print stuff */
	log_print_parent_and_child_stacks(parent_tcb, child_tcb );

    /* After setting up child stack and VM, register with scheduler */
    if (register_thread(child_tcb->tid) < 0)
        return -1;

    /* Only parent will return here */
    assert(get_running_tid() == parent_tcb->tid);
	return child_tcb->tid;
}

/** @brief Installs the fork() interrupt handler
 */
int
install_fork_handler(int idt_entry, asm_wrapper_t *asm_wrapper)
{
	if (!asm_wrapper) {
		return -1;
	}
	init_fork();
	int res = install_handler_in_idt(idt_entry, asm_wrapper, DPL_3);
	return res;
}



