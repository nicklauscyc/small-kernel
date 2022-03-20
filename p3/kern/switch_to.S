

.globl switch_to

# switch_to ( esp, *old_esp, pdbr )
switch_to:
	movl 4(%esp), %eax /* save esp arg */
	movl 8(%esp), %ecx /* save *old_esp */
	movl 12(%esp), %edx /* save pdbr */
    pusha       # Store general-purpose registers
    push %ds    # Store data segment registers
    push %es
    push %fs
    push %gs

    movl %esp, (%ecx) # Store curr esp into old_esp pointer
    movl %edx, %cr3 # Update page-directory TODO: Can call function if we store esp in callee save reg (may need to OR with lower 12 bits
    movl %eax, %esp     # Set esp to new_esp

    pop %gs     # Restore registers from new thread
    pop %fs
    pop %es
    pop %ds
    popa          # restore general-purpose registers
    ret
