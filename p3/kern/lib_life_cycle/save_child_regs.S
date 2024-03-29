
.globl update_child_esp

# void update_child_esp(uint32_t *child_esp,uint32_t child_stack_lo,
#                       uint32_t parent_lowest)
update_child_esp:
	leal 4(%esp), %eax # save parent esp
	movl %eax, 4(%esp)



.globl save_child_regs
/* void *save_child_regs(void *parent_kern_esp, void *child_kern_esp
 *						 void *child_cr0, void *child_cr3 ); */
save_child_regs:
	/**************************************************************************/
	/* header: necessary steps to guarantee correct function return           */
	pushl %ebp /* save %ebp                                                   */
	movl %esp, %ebp /* update %ebp                                            */
	/**************************************************************************/

	movl 8(%ebp), %ecx /* %ecx holds parent_kern_esp, points to parent tid */
	movl 12(%ebp), %esp /* %esp set to child stack highest address */

    /* This part copies relevant parts of the parent stack to child stack.
	 * Although tedious, we avoid loops for clarity
	 */
	movl -4(%ecx), %edx /* parent SS in %edx since fork() privilege change */
	pushl %edx
	movl -8(%ecx), %edx /* parent ESP in %edx since fork() privilege change */
	pushl %edx
	movl -12(%ecx), %edx /* parent EFLAGS in %edx */
	pushl %edx
	movl -16(%ecx), %edx /* parent CS in %edx */
	pushl %edx
	movl -20(%ecx), %edx /* parent EIP in %edx */
	pushl %edx
	movl -24(%ecx), %edx /* parent ebp in %edx from call_fork() */
	pushl %edx
	movl -28(%ecx), %edx /* parent edi in %edx from call_fork() */
	pushl %edx
	movl -32(%ecx), %edx /* parent ebx in %edx from call_fork() */
	pushl %edx
	movl -36(%ecx), %edx /* parent esi in %edx from call_fork() */
	pushl %edx
	movl -40(%ecx), %edx /* parent ds in %edx from call_fork() */
	pushl %edx
	movl -44(%ecx), %edx /* parent es in %edx from call_fork() */
	pushl %edx
	movl -48(%ecx), %edx /* parent fs in %edx from call_fork() */
	pushl %edx
	movl -52(%ecx), %edx /* parent gs in %edx from call_fork() */
	pushl %edx
	movl -56(%ecx), %edx /* return address to call_fork() in %edx */
	pushl %edx

	/* This part onwards must match with context switcher */
	leal 32(%esp), %edx /* parent ebp in %edx */
	pushl %edx

	/* stuff to store child tid in stack position for future eax value */
	movl $0, %edx

	pushl %edx

# TODO: Just subl 20 to child esp instead
#		Or just 5 times, subl 4 so we can comment what's up
	movl -68(%ecx), %edx /* parent ebx in %edx */
	pushl %edx
	movl -72(%ecx), %edx /* parent ecx in %edx */
	pushl %edx
	movl -76(%ecx), %edx /* parent edx in %edx */
	pushl %edx
	movl -80(%ecx), %edx /* parent edi in %edx */
	pushl %edx
	movl -84(%ecx), %edx /* parent esi in %edx */
	pushl %edx
	movl 16(%ebp), %edx /* parent cr0 in %edx */
	pushl %edx
	movl 20(%ebp), %edx /* parent cr3 in %edx */
	pushl %edx

	movl %esp, %eax /* return the stack pointer for child to use */

	/**************************************************************************/
	/* footer: necessary steps to guarantee correct function return           */
	movl %ebp, %esp /* %esp points to return address's address + 4            */
	popl %ebp /* restore %ebp, %esp points to return address                  */
	ret
	/**************************************************************************/


