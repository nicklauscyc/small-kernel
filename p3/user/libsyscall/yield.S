#include <syscall_int.h>

.globl yield

yield:
	/* Save all callee save registers */
	pushl %ebp
	movl  %esp, %ebp
	pushl %edi
	pushl %ebx
	pushl %esi

    movl 8(%ebp), %esi  /* Place argument in %esi */
	int  $YIELD_INT		/* Call handler in IDT for yield */

	/* Restore all callee save registers */
	popl %esi
	popl %ebx
	popl %edi
	popl %ebp
	ret
