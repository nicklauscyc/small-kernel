.globl iret_travel

# iret_travel ( eip, cs, eflags, esp, ss )
iret_travel:
    addl $4, %esp   # Point esp to last argument
    iret            # Consume all arguments and go to user mode!
