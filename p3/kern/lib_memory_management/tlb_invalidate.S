/** @file tlb_invalidate.S
 *  @brief Assembly function to invalidate a TLB entry
 */

.globl invalidate_tlb

# tlb_invalidate( void *addr )
invalidate_tlb:
	movl 4(%esp), %eax # Put addr in eax
	invlpg (%eax)
	ret
