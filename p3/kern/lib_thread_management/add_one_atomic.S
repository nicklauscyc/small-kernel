.globl add_one_atomic

/* int add_one_atomic(int *at) */
add_one_atomic:
    movl 4(%esp), %ecx      /* Move address argument to ecx */
    movl $1, %eax           /* eax = 1 */
    lock xaddl %eax, (%ecx) /* Atomically: temp = eax + *ecx; eax = *ecx; *ecx = temp */
    ret
