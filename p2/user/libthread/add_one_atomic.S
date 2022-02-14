.globl add_one_atomic

add_one_atomic:
    movl 8(%ebp), %ebx      /* Move address argument to ebx */
    movl $1, %eax           /* eax = 1 */
    lock
    xaddl %eax, (%ebx) /* Atomically: eax = *ebx and *ebx = *ebx + 1 */
    ret
