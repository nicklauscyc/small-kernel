.globl swexn_set_regs

# swexn_set_regs( ureg_t *newureg )
swexn_set_regs:
	movl 4(%esp), %eax # eax = newureg

	/* Set data segment registers*/
    movl 8(%eax), %cx
    movl %cx, %ds
    movl 12(%eax), %cx
	movl %cx, %es
    movl 16(%eax), %cx
	movl %cx, %fs
    movl 20(%eax), %cx
	movl %cx, %gs

	/* Set GP registers */
	movl 24(%eax), %edi
	movl 28(%eax), %esi
	movl 32(%eax), %ebp
	# Skip esp, since that's set by iret
	movl 40(%eax), %ebx
	movl 44(%eax), %edx
	movl 48(%eax), %ecx
	# Skip eax, can only set eax at the end, since it points to newureg

	/* Set up stack for iret */
	pushl 76(%eax) # ss
	pushl 72(%eax) # esp
	pushl 68(%eax) # eflags
	pushl 64(%eax) # cs
	pushl 60(%eax) # eip

	movl 52(%eax), %eax # Finally, set eax (since we no longer need newureg)

	iret


# maybe set registers then do the same as iret travel?
# set up stack for iret travel, then update all GP regs

