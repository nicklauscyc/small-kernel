.globl compare_and_swap_atomic
.globl add_one_atomic

/* int add_one_atomic( int *at ) */
add_one_atomic:
    movl 4(%esp), %ecx      /* Move address argument to ecx */
    movl $1, %eax           /* eax = 1 */
    lock xaddl %eax, (%ecx) /* Atomically: temp = eax + *ecx; eax = *ecx; *ecx = temp */
    ret

/* int CAS( int *at, int expect, int new_val ) */
compare_and_swap_atomic:
	mov 8(%esp), %eax			/* Get expect into ax */
	mov 12(%esp), %ecx			/* Get new_val into cx */
	lock cmpxchg %ecx, 4(%esp)	/* Atomically: if (expect == *addr)
											then %ax = *addr, *addr = new_val */
	ret
