.globl iret_travel

# iret_travel (ss, esp, eflags, cs, eip )
iret_travel:
    subl $4, %esp   # Point esp to last argument
    iret            # Consume all arguments and go to user mode!
