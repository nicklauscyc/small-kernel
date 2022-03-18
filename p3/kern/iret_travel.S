#include <seg.h> /* SEGSEL_USER_DS */

.globl iret_travel

# iret_travel ( eip, cs, eflags, esp, ss )
iret_travel:
    movl $SEGSEL_USER_DS, %ax
    movl %ax, %ds   # Update ds to user data segment
	movl %ax, %es
	movl %ax, %fs
	movl %ax, %gs
    addl $4, %esp   # Point esp to last argument
    iret            # Consume all arguments and go to user mode!
