
.globl save_child_regs

save_child_regs: /* set up for context switch */
	movl %esp, %ecx /* save current esp */
	movl 4(%esp), %esp /* update esp */
	pushl %ebp
	movl %esp, %ebp /* update %ebp */
	pushl %eax /* need to save all since that's waht context switch does */
	pushl %ebx
	pushl %ecx
	pushl %edx
	pushl %edi
	pushl %esi
	movl %esp, %eax /* return the new %esp */
	movl %ecx, %esp /* restore %esp */
	ret

