/** @file thread_fork.S
 *  @brief Assembly wrapper for the thread_fork() system call
 *  @author Andre Nascimento (anascime)
 *  @bugs No known bugs
 */

#include <syscall_int.h>

.globl thread_fork

//thread_fork:
//	/* Save all callee save registers on parent stack */
//	pushl %ebp
//	movl  %esp, %ebp
//	pushl %edi
//	pushl %ebx
//	pushl %esi
//
//    /* Save all callee save registers on child stack */
//    movl %esp, %eax         /* Save our esp on eax temporarily */
//    movl 8(%ebp), %esp      /* Move future child stack pointer into our esp */
//    /* We don't save ebp and esp as those are different btwn child and parent */
//    pushl %edi
//    pushl %ebx
//    pushl %esi
//    movl %eax, %esp         /* Restore parent esp */
//    movl %eax, %ebx         /* Store child esp on ebx */
//
//	int  $THREAD_FORK_INT   /* Call handler in IDT for thread_fork() */
//
//    cmp $0, %eax            /* If tid == 0 ... */
//    je child_continue       /* Go to child procedure, else */
//
//	/* Parent procedure: restore all callee save registers on parent stack */
//	popl %esi
//	popl %ebx
//	popl %edi
//	popl %ebp
//	ret

thread_fork:
    movl 4(%esp), %ecx      /* Move future child stack pointer into ecx */
    movl (%esp), %edx       /* Put return addres into edx*/

	int  $THREAD_FORK_INT   /* Call handler in IDT for thread_fork() */

    cmp $0, %eax            /* If tid == 0 ... */
    je child_continue       /* Go to child procedure, else... */
    ret                     /* we're done. */

child_continue:
    movl %ecx, %esp     /* Update our child esp (previously stored in ebx). */
    movl %esp, %ebp     /* TODO: Is this necessary? */

    jmpl *%edx
