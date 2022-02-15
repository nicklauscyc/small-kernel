/** @file thread_fork.S
 *  @brief Assembly wrapper for the thread_fork() system call
 *  @author Andre Nascimento (anascime)
 *  @bugs No known bugs
 */

#include <syscall_int.h>
#include <thread.h>

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

/*
 *
	movl 8(%ebp), %esp
	movl 16(%ebp), %esi
	pushl %esi
	movl 12(%ebp), %ebx
	calll *%ebx
 */

thread_fork:
    movl 4(%esp), %ecx      /* Move future child stack pointer into ecx */
    movl 8(%esp), %edx      /* Move func into edx */

    /* Push arguments into child stack */
    movl %esp, %eax         /* Store parent esp in eax */
    movl %ecx, %esp         /* Go to child stack */
    pushl 12(%eax)          /* Push arg */
    movl %eax, %esp         /* Restore parent stack pointer */

	int  $THREAD_FORK_INT   /* Call handler in IDT for thread_fork() */

    cmp $0, %eax            /* If tid == 0 ... */
    je child_continue       /* Go to child procedure, else... */
    ret                     /* we're done. */


child_continue:
    leal -4(%ecx), %esp /* Update our child esp (previously stored in ecx). */
    movl %esp, %ebp     /* TODO: Is this necessary? */
    calll *%edx         /* Call func, previously stored in edx */
    pushl %eax          /* Push return value onto stack for call to thr_exit */
    call thr_exit

    /* Thr_exit should never never return, assert(false) */
    //pushl $0
    //call assert

