/** @file thread_fork.S
 *  @brief Assembly wrapper for the thread_fork() system call
 *  @author Andre Nascimento (anascime)
 *  @bugs No known bugs
 */

#include <syscall_int.h>

.globl thread_fork

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
	movl %edx, %edi     /* save the function to call later */
    leal -4(%ecx), %esp /* Update our child esp (previously stored in ecx). */
    movl %esp, %ebp     /* Update %ebp since we use base pointers */
	leal 4(%ebp), %ecx  /* Calculate top of exn stack */
	pushl %ecx          /* Push top of exn stack address for argument setup */
	calll install_child_pf_handler /* Install child's pagefault handler */
	addl $4, %esp       /* Restore %esp to deallocate arg space for pf handler*/
    calll *%edi         /* Call func, previously stored in edx */
    pushl %eax          /* Push return value onto stack for call to thr_exit */
    call thr_exit

    /* Thr_exit should never never return, panic */
    call panic

