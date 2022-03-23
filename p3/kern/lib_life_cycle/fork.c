/** @file fork.c
 *  @brief Contains fork interrupt handler and helper functions for
 *  	   installation
 *
 *
 */

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
#include <memory_manager.h> /* new_pd_from_parent */
#include <simics.h>
#include <scheduler.h>

// saves regs and returns new esp
void *save_child_regs(void *parent_kern_esp, void *child_kern_esp);

void
init_fork( void )
{
	/* honestly not sure what to init */
}

/** @brief Fork task into two.
 *
 *  This can be called on a task with multiple threads, however, only
 *  the calling thread will be duplicated to the child task
 *
 *  @return 0 for child thread, pid for parent thread */
int
fork( void )
{
	/* TODO This thing needs to defer execution of copy till later? */
	uint32_t cr3 = get_cr3();
	uint32_t *parent_pd = (uint32_t *) (cr3 & ~(PAGE_SIZE - 1));

	/* parent_pd in kernel memory, unaffected by paging */
	assert((uint32_t) parent_pd < USER_MEM_START);

    uint32_t *child_pd = new_pd_from_parent((void *)parent_pd);

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
    affirm((child_tcb = find_tcb(child_tid)) != NULL);

#ifndef NDEBUG
    /* Register this task with simics for better debugging */
	// TODO what is elf->e_fname for this guy?
    //sim_reg_process(pd, elf->e_fname);
#endif

	// Duplicate parent kern stack in child kern stack
	tcb_t *parent_tcb;
	assert((parent_tcb = find_tcb(get_running_tid())) != NULL);

	/* Acknowledge interrupt and return */
	/* TODO: Acknowledge interrupt quickly! Before any allocation is made */
    outb(INT_CTL_PORT, INT_ACK_CURRENT);

	uint32_t *child_kernel_esp_on_ctx_switch;
	child_kernel_esp_on_ctx_switch = save_child_regs(parent_tcb->kernel_esp,
	                                                 child_tcb->kernel_esp);
	child_tcb->kernel_esp = child_kernel_esp_on_ctx_switch;

    /* After setting up child stack and VM, register with scheduler */
    register_thread(child_tcb->tid);

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



