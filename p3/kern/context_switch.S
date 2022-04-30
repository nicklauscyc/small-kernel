/** @file context_switch.S */

#include <seg.h>

/**
 * Since we direct map kernel memory, changing the value in cr3 should
 * have no effect on the stack values being stored on the kernel stack
 */
.globl context_switch

context_switch:
	pushl %ebp
	movl %esp, %ebp /* update %ebp */
	pushl %eax
	pushl %ebx
	pushl %ecx
	pushl %edx
	pushl %edi
	pushl %esi

	movl %cr0, %ecx
	pushl %ecx

	movl %cr3, %ecx
	pushl %ecx

	movl 8(%ebp), %ecx
	movl %esp, (%ecx)	/* save current %esp to first arg */
	movl 12(%ebp), %esp /* restore %esp from second arg */

	popl %ecx
	movl %ecx, %cr3		/* update page directory */

	popl %ecx
	movl %ecx, %cr0		/* update page directory */

	popl %esi
	popl %edi
	popl %edx
	popl %ecx
	popl %ebx
	popl %eax
	popl %ebp

	sti		/* enable interrupts */

	ret

