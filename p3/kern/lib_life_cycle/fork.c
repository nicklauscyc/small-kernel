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

void
init_fork( void )
{
	/* honestly not sure what to init */
}

int
fork( void )
{
	/* TODO This thing needs to defer execution of copy till later? */
	uint32_t *parent_pd = (uint32_t *) get_cr3();

	/* parent_pd in kernel memory, unaffected by paging */
	assert((uint32_t) parent_pd < USER_MEM_START);

	/* child_pd is a shallow copy of parent_pd */
	uint32_t *child_pd = smemalign(PAGE_SIZE, PAGE_SIZE);
	if (!child_pd) {
		return -1;
	}
	assert((uint32_t) child_pd < USER_MEM_START);
	memset(child_pd, 0, PAGE_SIZE);
	for (int i = 0; i < PAGE_SIZE; i++) {
		child_pd[i] = parent_pd[i];
	}



    /* Acknowledge interrupt and return */
    outb(INT_CTL_PORT, INT_ACK_CURRENT);

    /* TODO: Just return 0 for now. Later, get
     * current thread from scheduler. */
    return 0;
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



