
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
	movl %esp, 8(%ebp) /* save current %esp to first arg */
	movl 12(%ebp), %esp /* restore %esp from second arg */
	popl %esi
	popl %edi
	popl %edx
	popl %ecx
	popl %ebx
	popl %eax
	popl %ebp
	ret

