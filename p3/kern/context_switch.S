#include <seg.h>

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

	movl %cr3, %ecx
	pushl %ecx

	movl 8(%ebp), %ecx
	movl %esp, (%ecx)	/* save current %esp to first arg */
	movl 12(%ebp), %esp /* restore %esp from second arg */
	#movl 16(%ebp), %ecx /* put to_run->pd into ecx */

	popl %ecx
	movl %ecx, %cr3		/* update page directory */

	popl %esi
	popl %edi
	popl %edx
	popl %ecx
	popl %ebx
	popl %eax
	popl %ebp

	sti		/* enable interrupts */

	ret

