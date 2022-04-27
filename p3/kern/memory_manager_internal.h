/** @file memory_manager_internal.h
 *  @brief Internal macros and functions that memory related functions in
 *         lib_memory_manager/ use.
 */


/* System programmer flags.
 * Bits 9, 10, 11 are used together to offer 8 possible flags that cannot
 * be bit-ORed with one another
 */
#define NEW_PAGE_BASE_FLAG (1 << 9)
#define NEW_PAGE_CONTINUE_FROM_BASE_FLAG (2 << 9)

/* 7 is 111 in binary, and we bitshift << 9 to only keep bits 9, 10, 11 in the
 * address
 */
#define SYS_PROG_FLAG(ADDRESS)\
	(((uint32_t)(ADDRESS)) & (7 << 9))

#define PAGING_FLAG (1 << 31)
#define WRITE_PROTECT_FLAG (1 << 16)

#define PAGE_GLOBAL_ENABLE_FLAG (1 << 7)

#define PAGE_OFFSET 0x00000FFF

/* 16MB = 4 PT */
#define NUM_KERN_PAGE_TABLES 4

/* Flags for page directory and page table entries */
#define PRESENT_FLAG (1 << 0)
#define RW_FLAG		 (1 << 1)
#define USER_FLAG	 (1 << 2)
#define GLOBAL_FLAG  (1 << 8)

#define PE_USER_READABLE (PRESENT_FLAG | USER_FLAG )
#define PE_USER_WRITABLE (PE_USER_READABLE | RW_FLAG)

/* Set global flag so TLB doesn't flush kernel entries */
#define PE_KERN_READABLE (PRESENT_FLAG | GLOBAL_FLAG)
#define PE_KERN_WRITABLE (PE_KERN_READABLE | RW_FLAG)
#define PE_UNMAPPED 0

#define TABLE_ENTRY_INVARIANT(TABLE_ENTRY)\
	((((uint32_t)(TABLE_ENTRY) != 0) && (TABLE_ADDRESS(TABLE_ENTRY) != 0))\
	|| ((uint32_t)(TABLE_ENTRY) == 0))

#define SYS_ZERO_FRAME (USER_MEM_START)

uint32_t *get_ptep( const uint32_t **pd, uint32_t virtual_address );
int is_valid_sys_prog_flag( uint32_t sys_prog_flag );
void unallocate_frame( uint32_t **pd, uint32_t virtual_address );

