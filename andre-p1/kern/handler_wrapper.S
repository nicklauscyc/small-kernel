.globl asm_timer_wrapper
.globl asm_keyboard_wrapper

asm_keyboard_wrapper:
    pusha                 # Push general-purpose registers onto stack
    call keyboard_handler # Call keyboard handler
    popa                  # Pop general-purpose registers from stack
    iret                  # Return from interrupt handler

asm_timer_wrapper:
    pusha              # Push general-purpose registers onto stack
    call timer_handler # Call timer handler
    popa               # Pop general-purpose registers from stack
    iret               # Return from interrupt handler
