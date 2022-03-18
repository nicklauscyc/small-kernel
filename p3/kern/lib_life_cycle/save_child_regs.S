
.globl update_child_esp

# void update_child_esp(uint32_t *child_esp,uint32_t child_stack_lo, uint32_t parent_lowest)
update_child_esp:
	leal 4(%esp), %eax # save parent esp
	movl %eax, 4(%esp)



.globl save_child_regs

save_child_regs: /* set up for context switch */

	movl %esp, %ecx /* save current esp from fork () */
	/* save ret address onto child stack */
	//movl (%esp), %ecx /* save ret addresss */
	//movl 4(%esp), %edx /* memory of new esp */
	//subl $4, %edx
	//movl %ecx, (%edx)
	//movl 4(%esp), %esp /* update esp to child pointer */
	//pushl (%esp) /* ret address of fork() right before this call */
	//pushl %ebp
	//movl %esp, %ebp /* update %ebp */
	//pushl %eax /* need to save all since that's waht context switch does */
	//pushl %ebx
	//pushl %ecx
	//pushl %edx
	//pushl %edi
	//pushl %esi
	//movl %esp, %eax /* return the new %esp */
	movl %ecx, %esp /* restore %esp */
	//movl %ecx, %ebp /* restore %ebp */
	ret

