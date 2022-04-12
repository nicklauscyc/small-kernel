/** @file memory_manager_internal.h
 *  @brief Internal macros and functions that memory related functions in
 *         lib_memory_manager/ use.
 */


#define PAGING_FLAG (1 << 31)
#define WRITE_PROTECT_FLAG (1 << 16)

#define PAGE_GLOBAL_ENABLE_FLAG (1 << 7)

#define PAGE_OFFSET 0x00000FFF

/* Flags for page directory and page table entries */
#define PRESENT_FLAG (1 << 0)
#define RW_FLAG		 (1 << 1)
#define USER_FLAG	 (1 << 2)
#define GLOBAL_FLAG  (1 << 8)

#define PE_USER_READABLE (PRESENT_FLAG | USER_FLAG )
#define PE_USER_WRITABLE (PE_USER_READABLE | RW_FLAG)

/* Set global flag so TLB doesn't flush kernel entries */
#define PE_KERN_READABLE (PRESENT_FLAG | GLOBAL_FLAG | RW_FLAG)
#define PE_KERN_WRITABLE (PE_KERN_READABLE | RW_FLAG)
#define PE_UNMAPPED 0

#define TABLE_ENTRY_INVARIANT(TABLE_ENTRY)\
	((((uint32_t)(TABLE_ENTRY) != 0) && (TABLE_ADDRESS(TABLE_ENTRY) != 0))\
	|| ((uint32_t)(TABLE_ENTRY) == 0))

