#include <syscall_int.h>

.globl run_test

run_test:
	/* Save all callee save registers */
	pushl %ebp
	movl  %esp, %ebp
	pushl %edi
	pushl %ebx
	pushl %esi

    movl 8(%ebp), %esi          /* Place argument in %esi */
	int  $SYSCALL_RESERVED_0	/* Call handler in IDT for run_tests(TEST_NUM) */

	/* Restore all callee save registers */
	popl %esi
	popl %ebx
	popl %edi
	popl %ebp
	ret
