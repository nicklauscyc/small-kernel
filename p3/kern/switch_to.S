
  
.globl switch_to

# switch_to ( esp, *old_esp, pdbr )
switch_to:
    pusha       # Store general-purpose registers
    push %ds    # Store data segment registers
    push %es
    push %fs
    push %gs

    movl %esp, 56(%esp) # Store curr esp into old_esp pointer
    movl 52(%esp), %eax # Temporarily store new_esp in eax
    movl 60(%esp), %ebx
    movl %ebx, %cr3 # Update page-directory TODO: Can call function if we store esp in callee save reg
    movl %eax, %esp     # Set esp to new_esp

    pop %gs     # Restore registers from new thread
    pop %fs
    pop %es
    pop %ds
    popa          # restore general-purpose registers
    ret
