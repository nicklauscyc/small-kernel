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
#include <x86/page.h> /* PAGE_SIZE */
#include <task_manager.h> /* TODO this must be deleted after demo, breaks interface */
#include <simics.h>

// saves regs and returns new esp
void *save_child_regs(void * kernel_esp);

void
init_fork( void )
{
	/* honestly not sure what to init */
}

int
fork( void )
{
	/* TODO This thing needs to defer execution of copy till later? */
	uint32_t cr3 = get_cr3();
	lprintf("fork() thinks cr3 is:%lx", cr3);
	uint32_t *parent_pd = (uint32_t *) (cr3 & ~(PAGE_SIZE - 1));
	lprintf("parent_pd:%p", parent_pd);

	/* parent_pd in kernel memory, unaffected by paging */
	assert((uint32_t) parent_pd < USER_MEM_START);

	/* child_pd is a shallow copy of parent_pd */
	uint32_t *child_pd = smemalign(PAGE_SIZE, PAGE_SIZE);
	if (!child_pd) {
		return -1;
	}
	assert((uint32_t) child_pd < USER_MEM_START);
	memset(child_pd, 0, PAGE_SIZE);
	for (int i = 0; i < (PAGE_SIZE / sizeof(uint32_t)); i++) {
		child_pd[i] = parent_pd[i];
	}
	/* TODO hard code child pid, tid to 1 */
    if (get_new_pcb(1, child_pd) < 0)
        return -1;

    /* TODO: Deallocate pcb if this fails */
	tcb_t *child_tcb = (tcb_t *) get_new_tcb(1, 1);
    //if (get_new_tcb(1, 1) < 0)
    //    return -1;
#ifndef NDEBUG
    /* Register this task with simics for better debugging */
	// TODO what is elf->e_fname for this guy?
    //sim_reg_process(pd, elf->e_fname);
#endif

	// Duplicate parent kern stack in child kern stack
	tcb_t *parent_tcb;
	assert(find_tcb(0, &parent_tcb) == 0);

	/* Acknowledge interrupt and return */
    outb(INT_CTL_PORT, INT_ACK_CURRENT);

	memcpy(child_tcb->kernel_stack_lowest_address,
		parent_tcb->kernel_stack_lowest_address,
		PAGE_SIZE);

	lprintf("before save_child_regs");
	MAGIC_BREAK;
	child_tcb->kernel_esp = save_child_regs(child_tcb->kernel_esp);
	lprintf("after save_child_regs");
	// after this point, both child and parent will run this code




	// This is where esp should diverge

    /* TODO: Just return 0 for now. Later, get
     * current thread from scheduler. */
    return 1; // child TID HARDCODED
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



