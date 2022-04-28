/** @file memory_manager.c
 *	@brief Functions to initialize and manage virtual memory
 *
 *  Note that page directory and page table consistency/validity checks are
 *  expensive and thus carried out in assertions.
 *
 *
 *	@author Andre Nascimento (anascime)
 *	@author Nicklaus Choo (nchoo)
 *
 * */

#include <simics.h>
#include <physalloc.h> /* physalloc() */
#include <memory_manager.h>
#include <stdint.h>		/* uint32_t */
#include <stddef.h>		/* NULL */
#include <malloc.h>		/* smemalign, sfree */
#include <elf_410.h>	/* simple_elf_t */
#include <assert.h>		/* assert, affirm */
#include <page.h>		/* PAGE_SIZE */
#include <x86/cr.h>		/* {get,set}_{cr0,cr3} */
#include <string.h>		/* memset, memcpy */
#include <common_kern.h>/* USER_MEM_START */
#include <logger.h>		/* log */
#include <memory_manager_internal.h>
#include <lib_thread_management/mutex.h> /* mutex_init */


/* 1 if VM is enabled, 0 otherwise */
static int paging_enabled = 0;

/* Initial page directory that maps all of kernel memory, aliases across all
 * other page directory's lowest 4 indexed page tables */
static uint32_t **initial_pd = NULL;

static int allocate_frame( uint32_t **pd, uint32_t virtual_address,
                           write_mode_t write_mode, uint32_t sys_prog_flag );
static int allocate_region( uint32_t **pd, void *start, uint32_t len,
                            write_mode_t write_mode );
static void enable_paging( void );

static void vm_set_pd( void *pd );
static void free_pt_memory( uint32_t *pt, int pd_index );

static void *allocate_new_pd( void );
static int add_new_pt_to_pd( uint32_t **pd, uint32_t virtual_address );


/** @brief Initializes the memory manager functions and creates the initial
 *         zero frame and initial page directory.
 *
 *  @return Void.
 */
void
init_memory_manager( void )
{
	mutex_init(&pages_mux);
	initialize_zero_frame();
	create_initial_pd();
}

/** @brief returns the address of the initial page directory
 *
 *  @return The initial page directry address that is guaranteed non-NULL
 */
void *
get_initial_pd( void )
{
	affirm(initial_pd);
	return (void *) TABLE_ADDRESS(initial_pd);
}

/** @brief Creates the initial page directory which creates the page tables
 *         for the lowest 4 indices of the page table. Direct maps all
 *         kernel memory with correct read and write permissions.
 *
 *  @return Void.
 */
void
create_initial_pd( void )
{
	initial_pd = allocate_new_pd();
	affirm_msg(initial_pd, "creat_initial_pd(): Unable to allocate memory for "
	           "initial page directory.");

    /* Direct map all 16MB for kernel, setting correct permission bits */
    for (uint32_t addr = 0; addr < USER_MEM_START; addr += PAGE_SIZE) {

		/* This invariant must not break */
		uint32_t pd_index = PD_INDEX(addr);
		uint32_t *pd_entry = initial_pd[pd_index];
		affirm_msg(TABLE_ENTRY_INVARIANT(pd_entry),
				   "creat_initial_pd(): "
				   "pd entry invariant broken for "
				   "pd:%p pd_index:0x%08lx pd[pd_index]:%p",
				   initial_pd, pd_index, pd_entry);

		/* Add new page table every time page directory entry is NULL */
		if (initial_pd[pd_index] == NULL) {

			/* Since we are going in increasing virtual addresses, holds */
			assert((addr & ((1 << PAGE_DIRECTORY_SHIFT) - 1)) == 0);
			affirm_msg(add_new_pt_to_pd(initial_pd, addr) == 0,
				       "create_initial_pd(): "
                       "unable to allocate new page table in pd:%p for "
                       "virtual_address: 0x%08lx", initial_pd, addr);
		}
		/* Now get a pointer to the corresponding page table entry */
		uint32_t *ptep = get_ptep((const uint32_t **) initial_pd, addr);

		/* TODO If crashing we don't clean up right */
		/* If NULL is returned, free all resources in page directory */
		affirm_msg(ptep, "create_initial_pd(): "
                   "unable to get page table entry pointer.");

		/* Indicate page table entry permissions */
		if (addr == 0) {
			*ptep = addr | PE_UNMAPPED; /* Leave NULL unmapped. */
		} else {
			*ptep = addr | PE_KERN_WRITABLE;
		}
		assert(*ptep < USER_MEM_START);
	}
	/* Contractually ensure that the initial pd is correct on startup */
	affirm(is_valid_pd(initial_pd));
}


/** @brief Unallocates a physical frame associated with a given virtual
 *         address. Flushes the TLB entry by using INVLPG after updating the
 *         page directory.
 *
 *  @param pd Page directory from which to free the physical frame from
 *  @param virtual_address Virtual address mapped to a physical frame
 *  @return Void.
 */
void
unallocate_frame( uint32_t **pd, uint32_t virtual_address )
{
	uint32_t *ptep = get_ptep((const uint32_t **) pd, virtual_address);
	affirm_msg(ptep, "unallocate_frame(): "
			  "cannot free non existent page table, "
			  "pd:%p, "
			  "virtual_address:0x%08lx",
			  pd, virtual_address);
	uint32_t pt_entry = *ptep;

	affirm(pt_entry & PRESENT_FLAG);

	uint32_t phys_address = TABLE_ADDRESS(pt_entry);

	/* Only physfree physically allocated frames (as opposed to ZFOD frames) */
	if (phys_address != SYS_ZERO_FRAME)
		physfree(phys_address);

	/* Zero the entry as well */
	*ptep = 0;

	invalidate_tlb((void *)virtual_address);
}


/** @brief Returns pointer to page directory from cr3(), guarantees that pointer
 *		   is non-NULL and page aligned, and below USER_MEM_START
 *
 *	Does consistency check for valid page directory if NDEBUG is not defined.
 *
 *	@return Pointer to current active page directory
 */
void *
get_pd( void )
{
	void *pd = (void *) TABLE_ADDRESS(get_cr3());

	/* Basic checks for non-NULL, page aligned and < USER_MEM_START */
	affirm_msg(pd, "unable to get page directory");
	affirm_msg(PAGE_ALIGNED(pd), "page directory not page aligned!");
	affirm_msg((uint32_t) pd < USER_MEM_START,
			   "page directory > USER_MEM_START");

	/* Expensive check, hence the assertion */
	assert(is_valid_pd(pd));
	return pd;
}

/** @brief Handles page faults and allocates a physical frame if the faulting
 *         address references a page table entry that is a system wide zero
 *         frame.
 *
 *  @param faulting_address VM address that caused the page fault.
 *  @return 0 on success, negative value on error.
 */
int
zero_page_pf_handler( uint32_t faulting_address )
{
	/* get_pd() guarantees basic consistency for valid page directory */
	uint32_t **pd = get_pd();
	affirm(pd);

	uint32_t *ptep = get_ptep( (const uint32_t **) pd, faulting_address);

	/* Page table entry cannot be NULL frame. If it is, definitely did not
	 * allocate a system wide zero frame before so return -1 */
	if (!ptep) {
		log_warn("zero_page_pf_handler(): "
                 "page table entry for vm 0x%08lx is NULL!",
				 faulting_address);
		return -1;
	}
	uint32_t pt_entry = *ptep;

	/* Page table entry must hold the system wide zero frame.
	 * If not, then this is not a ZFOD allocated frame. Not our job to allocate
	 * a new physical frame to this entry */
	if (TABLE_ADDRESS(pt_entry) != SYS_ZERO_FRAME) {
		return -1;
	}
	/* Page table entry must be user readable since sys wide zero frame */
	affirm((pt_entry & PE_USER_READABLE) == PE_USER_READABLE);

	/* Get sys_prog_flag which must be valid, else state corrupted and we
	 * crash */
	uint32_t sys_prog_flag = SYS_PROG_FLAG(pt_entry);
	affirm(is_valid_sys_prog_flag(sys_prog_flag));

	/* Unallocate zero frame */
	unallocate_frame(pd, faulting_address);

	/* Back up with actual frame */
	int res = allocate_frame(pd, faulting_address, READ_WRITE, sys_prog_flag);
	if (res) {
		log_warn("zero_page_pf_handler(): "
		         "Failed to allocate frame inside zero_page_pf_handler");
		return -1;
	}

	/* Flush TLB so we zero out the appropriate physical frame */
	invalidate_tlb((void *)faulting_address);
	log("memsetting faulting_address %p, (table addr %p) to 0.",
			(void *)faulting_address,(void *)TABLE_ADDRESS(faulting_address));
	memset((void *)TABLE_ADDRESS(faulting_address), 0, PAGE_SIZE);

	return 0;
}

/** @brief Initializes the system wide zero frame. To be done before paging
 *         is enabled on kernel startup.
 *
 *  @return Void.
 */
void
initialize_zero_frame( void )
{
	affirm(!paging_enabled);

	/* Zero fill system wide zero frame */
	memset((uint32_t *) SYS_ZERO_FRAME, 0, PAGE_SIZE);
}

/** @brief Sets up a new page directory by allocating physical memory for it.
 *		   Does not transfer executable data into physical memory.
 *
 *	@return Valid page directory that is backed by physical memory, or NULL
 *	        if unable
 */
void *
new_pd_from_elf( simple_elf_t *elf )
{
	/* Allocate new pd that is zero filled */
    uint32_t **pd = allocate_new_pd();
	if (!pd) {
		log_warn("new_pd_from_elf(): "
		         "unable to allocate new page directory.");
		return NULL;
	}

	/* If there is a current page directory, the bottom 4 must be kernel mapped
	 * so use those instead
	 */
	affirm(initial_pd);
	uint32_t ** current_pd = (uint32_t **) TABLE_ADDRESS(initial_pd);
	for (int i = 0; i < NUM_KERN_PAGE_TABLES; ++i) {
		pd[i] = current_pd[i];
	}
	log("new_pd_from_elf(): direct map ended");
    /* Allocate regions with appropriate read/write permissions. */
    int i = 0;
	i += allocate_region(pd, (void *)elf->e_txtstart, elf->e_txtlen, READ_ONLY);
	i += allocate_region(pd, (void *)elf->e_datstart, elf->e_datlen,
	                     READ_WRITE);
	i += allocate_region(pd, (void *)elf->e_rodatstart, elf->e_rodatlen,
	                     READ_ONLY);
	i += allocate_region(pd, (void *)elf->e_bssstart, elf->e_bsslen,
	                     READ_WRITE);

	if (i < 0) {
		free_pd_memory(pd);
		sfree(pd, PAGE_SIZE);
		return NULL;
	}
	affirm(is_valid_pd(pd));
	return pd;
}

/** @brief Initialized child pd from parent pd. Deep copies writable
 *		   entries, allocating new physical frame. Returns child_pd on success
 *
 *  @param v_parent_pd Address of parent page directory
 *  @return Pointer to new child page directory on success, NULL on failure.
 */
void *
new_pd_from_parent( void *v_parent_pd )
{
	uint32_t *parent_pd = (uint32_t *)v_parent_pd;
	uint32_t *child_pd = smemalign(PAGE_SIZE, PAGE_SIZE);
	if (!child_pd) {
		return NULL;
	}
	memset(child_pd, 0, PAGE_SIZE); /* Set all entries to empty initially */

	/* Create temp_buf for deep copy. Too large to stack allocate */
	uint32_t *temp_buf = smemalign(PAGE_SIZE, PAGE_SIZE);
	if (!temp_buf) {
		sfree(child_pd, PAGE_SIZE);
		return NULL;
	}

	/* Just shallow copy kern memory page tables */
	for (int i=0; i < (PAGE_SIZE / sizeof(uint32_t)); ++i) {
		if (i < NUM_KERN_PAGE_TABLES) {
			child_pd[i] = parent_pd[i];
			continue;
		}

		if (parent_pd[i] & PRESENT_FLAG) {
			/* Allocate new child page_table */
			uint32_t *child_pt = smemalign(PAGE_SIZE, PAGE_SIZE);
			if (!child_pt) {
				free_pd_memory(child_pd); // Cleanup previous allocs
				sfree(temp_buf, PAGE_SIZE);
				sfree(child_pd, PAGE_SIZE);
				return NULL;
			}
			memset(child_pt, 0, PAGE_SIZE);
			assert(PAGE_ALIGNED(child_pt));
			log("child_pt:%p", child_pt);

			/* update child_pd[i] */
			child_pd[i] = (uint32_t) child_pt;
			/* OR the flags */
			child_pd[i] |= (parent_pd[i] & (PAGE_SIZE - 1));

			/* Get address of parent_pt */
			uint32_t *parent_pt = (uint32_t *)(parent_pd[i] & ~(PAGE_SIZE - 1));
			assert(PAGE_ALIGNED(parent_pt));

			/* parent_pt and child_pt are actual addresses without flags */

			/* Copy entries in page tables */
			for (int j=0; j < (PAGE_SIZE / sizeof(uint32_t)); ++j) {

				/* Constructing the virtual address */
				uint32_t vm_address =
					((i << PAGE_DIRECTORY_SHIFT) | (j << PAGE_TABLE_SHIFT));
				assert(PAGE_ALIGNED(vm_address));
				assert(vm_address >= USER_MEM_START);

				if (parent_pt[j] & PRESENT_FLAG) {
					/* Allocate new physical frame for child. */
					child_pt[j] = physalloc();
					assert((PAGE_ALIGNED(child_pt[j])));
					if (!child_pt[j]) {
						free_pd_memory(child_pd); // Cleanup previous allocs

						sfree(temp_buf, PAGE_SIZE);
						sfree(child_pd, PAGE_SIZE);
						return NULL;
					}

					/* Mark child_pt[j] as writable, copy and then mark
					 * same flags as the parent (in case READ-ONLY)
					 * Mark as user writable so we flush the entry to update
					 */
					child_pt[j] |= PE_USER_WRITABLE;

					log("vm_address:%lx, i:0x%x, j:0x%x, "
					"parent_pd[i]:0x%lx, child_pd[i]:0x%lx, "
					"parent_pt[j]:0x%lx, child_pt[j]:0x%lx",
					vm_address, i, j,
					parent_pd[i], child_pd[i],
					parent_pt[j], child_pt[j]);

					/* Copy parent to temp, change page-directory,
					 * copy child to parent, restore parent page-directory */
					memcpy(temp_buf, (uint32_t *) vm_address, PAGE_SIZE);
					vm_set_pd(child_pd);
					memcpy((uint32_t *) vm_address, temp_buf, PAGE_SIZE);
					vm_set_pd(parent_pd);

					/* Zero out flags */
					child_pt[j] &= ~(PAGE_SIZE - 1);
					/* Set parent flags */
					child_pt[j] |= parent_pt[j] & (PAGE_SIZE - 1);

				} else {
					assert(parent_pt[j] == 0);
				}
			}
		} else {
			assert(parent_pd[i] == 0);
		}
	}

	sfree(temp_buf, PAGE_SIZE);
	assert(is_valid_pd(child_pd));
	return child_pd;
}

/** @brief Sets new page table directory and enables paging if necessary.
 *
 *	Paging should only be set once and never disabled.
 *
 *	@param pd Page directory pointer
 *	@return Void.
 */
void
vm_enable_task( void *pd )
{
	affirm_msg(pd, "Page directory must be non-NULL!");
	affirm_msg(PAGE_ALIGNED(pd), "Page directory must be page aligned!");
	affirm_msg((uint32_t) pd < USER_MEM_START,
			   "Page directory must in kernel memory!");

	vm_set_pd(pd);
	if (!paging_enabled) {
		enable_paging();

		/* set PGE flag so kernel mappings not flushed on context switch */
		set_cr4(CR4_PGE | get_cr4());
	}
}

/** @brief Enables write_protect flag in cr0, stopping
 *	       kernel from bypassing VM's read-only protection
 *
 *	@return Void.
 */
void
enable_write_protection( void )
{
	uint32_t current_cr0 = get_cr0();
	set_cr0(current_cr0 | WRITE_PROTECT_FLAG);
}

/** @brief Disables write_protect flag in cr0, allowing kernel to bypass VM's
 *         read-only protection and copy data into memory for text section.
 *
 *  @return Void.
 */
void
disable_write_protection( void )
{
	uint32_t current_cr0 = get_cr0();
	set_cr0(current_cr0 & (~WRITE_PROTECT_FLAG));
}


/** @brief Checks if a user pointer is valid.
 *	Valid means the pointer is non-NULL, belongs to
 *	user memory and is in an allocated memory region.
 *
 *	@param ptr Pointer to check
 *	@param read_write What permission is needed
 *	@return 1 if valid, 0 if not */
int
is_valid_user_pointer(void *ptr, write_mode_t write_mode)
{
	/* Check not kern memory and non-NULL */
	if ((uint32_t)ptr < USER_MEM_START) {
		log_info("is_valid_user_pointer(): ptr:%p < USER_MEM_START",
		         ptr);
		return 0;
	}

	/* Check if allocated */
	if (!is_user_pointer_allocated(ptr)) {
		log_info("is_valid_user_pointer(): ptr:%p not allocated",
		         ptr);
		return 0;
	}

	/* Check for correct write_mode */
	uint32_t **pd = (uint32_t **)TABLE_ADDRESS(get_cr3());
	uint32_t pd_index = PD_INDEX(ptr);
	uint32_t pt_index = PT_INDEX(ptr);
	uint32_t *pt = (uint32_t *) TABLE_ADDRESS(pd[pd_index]);

	/* If looking for read write, ensure it's fully allocated or
	 * we have allocated with ZFOD. */
	if (write_mode == READ_WRITE && !((pt[pt_index] & RW_FLAG)
				|| TABLE_ADDRESS(pt[pt_index]) == SYS_ZERO_FRAME))
		return 0;

	if (write_mode == READ_ONLY && (pt[pt_index] & RW_FLAG))
		return 0;

	return 1;
}

/** @brief Checks if a user pointer is allocated.
 *
 *  @param ptr Pointer to be checked if allocated or not
 *  @return 1 if allocated, 0 otherwise.
 */
int
is_user_pointer_allocated( void *ptr )
{
	uint32_t **pd = (uint32_t **)TABLE_ADDRESS(get_cr3());
	uint32_t pd_index = PD_INDEX(ptr);
	uint32_t pt_index = PT_INDEX(ptr);

	/* Not present in page directory */
	if (!(((uint32_t) pd[pd_index]) & PRESENT_FLAG)) {
		return 0;
	}
	/* Not present in page table */
	uint32_t *pt = (uint32_t *) TABLE_ADDRESS(pd[pd_index]);
	if (!(pt[pt_index] & PRESENT_FLAG)) {
		return 0;
	}
	return 1;
}


/** @brief Checks that the address of every character in the string is a valid
 *		   address
 *
 *	The maximum permitted string length is USER_STR_LEN, including '\0'
 *	terminating character. Therefore the longest possible user string will
 *	have at most USER_STR_LEN - 1 non-NULL characters.
 *
 *	This does not check for the existence of a user executable with this
 *	name. That is done when we try to fill in the ELF header.
 *		   address. Does not check for write permissions!
 *
 *	@param s String to be checked
 *	@param len Max length of string
 *	@param null_terminated	Whether the string should be checked
 *							for null-termination
 *	@return 1 if valid user string, 0 otherwise
 */
static int
is_valid_user_string_helper( char *s, int len, int null_terminated)
{
	/* Check address of every character in s */
	int i;
	for (i = 0; i < len; ++i) {

		if (!is_valid_user_pointer(s + i, READ)) {
			log_warn("invalid address %p at index %d of user string %s",
					 s + i, i, s);
			return 0;

		} else {

			/* String ended early */
			if (s[i] == '\0') {
				break;
			}
		}
	}
	/* Check length of s */
	if (i == len && null_terminated) {
		log_warn("user string of length >= %d, not null-terminated", len);
		return 0;
	}
	return 1;
}

/** @brief Checks that the address of every character in the string is a valid
 *		   address. Does not check for write permissions!
 *
 *	The maximum permitted string length is max_len, including '\0'
 *	terminating character. Therefore the longest possible user string will
 *	have at most max_len - 1 non-NULL characters.
 *
 *	This does not check for the existence of a user executable with this
 *	name. That is done when we try to fill in the ELF header.
 *
 *	@param s String to be checked
 *	@param len Max length of string
 *	@return 1 if valid user string, 0 otherwise
 */
int
is_valid_null_terminated_user_string( char *s, int len )
{
	return is_valid_user_string_helper(s, len, 1);
}

/** @brief Checks that the address of every character in the string is a valid
 *		   address. Does not check for write permissions!
 *
 *	@param s String to be checked
 *	@param len Max length of string
 *	@return 1 if valid user string, 0 otherwise
 */
int
is_valid_user_string( char *s, int len )
{
	return is_valid_user_string_helper(s, len, 0);
}

/** @brief Checks address of every char * in argvec, argvec has max length
 *		   of < NUM_USER_ARGS
 *
 *	@param execname Executable name
 *	@param argvec Argument vector
 *	@return Number of user args if valid argvec, 0 otherwise
 */
int
is_valid_user_argvec( char *execname, char **argvec )
{
	/* Check address of every char * in argvec */
	int i;
	for (i = 0; i < NUM_USER_ARGS; ++i) {

		/* Invalid char ** */
		if (!is_valid_user_pointer(argvec + i, READ)) {
			log_warn("invalid address %p at index %d of argvec", argvec + i, i);
			return 0;

		/* Valid char **, so check if char * is valid */
		} else {

			/* String has ended within NUM_USER_ARGS */
			if (argvec[i] == NULL) {
				break;
			}
			/* Check if valid string */
			if (!is_valid_null_terminated_user_string(argvec[i],
						USER_STR_LEN)) {
				log_warn("invalid address user string %s at index %d of argvec",
						 argvec[i], i);
				return 0;
			}
		}
	}
	/* Check length of arg_vec */
	if (i == NUM_USER_ARGS) {
		log_warn("argvec has length >= NUM_USER_ARGS");
		return 0;
	}
	/* Check if argvec[0] == execname */
	if (strcmp(argvec[0],execname) != 0) {
		log_warn("argvec[0]:%s not equal to execname:%s", argvec[0], execname);
		return 0;
	}
	return i;
}

/* ----- HELPER FUNCTIONS ----- */

/** @brief Allocate memory for a new page table and zero all entries
 *
 *	Ensures that the allocated page table has an address that is page
 *	aligned
 *
 *	@return Pointer in kernel VM of page table if successful, 0 otherwise
 */
void *
allocate_new_pt( void )
{
	/* Allocate memory for a new page table */
	void *pt = smemalign(PAGE_SIZE, PAGE_SIZE);
	if (!pt) {
		return 0;
	}
	log("new pt at address %p", pt);
	affirm(PAGE_ALIGNED((uint32_t) pt));

	/* Initialize all page table entries as non-present */
	memset(pt, 0, PAGE_SIZE);

	return pt;
}

/** @brief Allocate memory for a new page directory and zero all entries
 *
 *  @return Pointer in kernel VM of page directory if successful, NULL otherwise
 */
static void *
allocate_new_pd( void )
{
	/* Allocate memory for a new page directory */
	void *pd = smemalign(PAGE_SIZE, PAGE_SIZE);
	if (!pd) {
		log_warn("allocate_new_pd(): "
		         "unable to allocate new page directory");
		return NULL;
	}
	log("allocate_new_pd(): "
	    "new pd at address %p", pd);
	affirm(PAGE_ALIGNED((uint32_t) pd));

	/* Initialize all page table entries as non-present */
	memset(pd, 0, PAGE_SIZE);

	return pd;
}

/** @brief Allocates a new page table and adds it to a page directory entry and
 *         initializes the page directory entry lower 12 bits to correct flags.
 *
 *  @param pd Page directory pointer to add the page table to.
 *  @param virtual_address VM address corresponding to the page
 *         table to be created and added to the page directory.
 *  @return 0 on success, -1 on error.
 */
static int
add_new_pt_to_pd( uint32_t **pd, uint32_t virtual_address )
{
	/* Page directory should be valid */
	assert(is_valid_pd(pd));

	/* pd cannot be NULL */
	if (!pd) {
		log_warn("add_new_pt_to_pd(): "
		         "pd cannot be NULL!");
		return -1;
	}
	/* Get page directory index */
    uint32_t pd_index = PD_INDEX(virtual_address);

	/* pd entry we're adding the new page table must be NULL */
	if (pd[pd_index] != NULL) {
		log_warn("add_new_pt_to_pd(): "
		         "pd_index:0x%08lx to insert into pd:%p for "
				 "virtual_address:0x%08lx must be NULL!, instead "
				 "pd[pd_index]:%p",
				 pd_index, pd, virtual_address, pd[pd_index]);
		return -1;
	}
	/* Allocate a new empty page table, set it to page table index */
	void *pt = allocate_new_pt();
	if (!pt) {
		log_warn("add_new_pt_to_pd(): "
		         "unable to allocate new page table in pd:%p for "
				 "virtual_address:0x%08lx", pd, virtual_address);
		return -1;
	}
	pd[pd_index] = pt;

	/* Page table should be valid */
	assert(is_valid_pt(pt, pd_index));

	/* Set all page directory entries as kernel writable, determine
	 * whether truly writable in page table entry. */
	pd[pd_index] = (uint32_t *)((uint32_t)pd[pd_index] | PE_USER_WRITABLE);

	/* Page directory should be valid */
	assert(is_valid_pd(pd));
	return 0;
}


/** @brief Gets pointer to page table entry in a given page directory.
 *		   Allocates page table if necessary.
 *
 *  Invariant violations for page directory or page table will cause
 *  a crash.
 *
 *	@param pd Page table address
 *	@param virtual_address Virtual address corresponding to page table
 *	@return Pointer to a page table entry that can be dereferenced to get
 *			a physical address, NULL on failure.
 */
uint32_t *
get_ptep( const uint32_t **pd, uint32_t virtual_address )
{
	/* Checking if a pd is valid is expensive, hence an assert() */
	assert(is_valid_pd(pd));

	/* NULL pd, so abort and return NULL */
	if (!pd) {
		log_warn("get_ptep(): "
		         "pd cannot be NULL!");
		return NULL;
	}
	/* Get page directory and page table index */
	uint32_t pd_index = PD_INDEX(virtual_address);
	uint32_t pt_index = PT_INDEX(virtual_address);

	/* Page directory cannot have NULL entry at corresponding page table */
	if (!pd[pd_index]) {
		log_warn("get_ptep(): "
		         "pd:%p, virtual_address:0x%08lx, pd[pd_index] cannot be "
				 "NULL!", pd, virtual_address);
		return NULL;
	}
	/* Page directory must have correct flag bits set */
    affirm_msg(((uint32_t) pd[pd_index] & PE_USER_WRITABLE) == PE_USER_WRITABLE,
			   "get_ptep(): "
		       "pd[pd_index]:%p does not have PE_USER_WRITABLE bits set!",
	            pd[pd_index]);

    /* Page table entry pointer zeroes out bottom 12 bits */
    uint32_t *ptep = (uint32_t *) TABLE_ADDRESS(pd[pd_index]);

	/* Return pointer to appropriate index */
	ptep += pt_index;
	affirm_msg(STACK_ALIGNED(ptep), "ptep:%p not stack aligned!", ptep);
	return ptep;
}

/** @brief Allocate new frame at given virtual memory address.
 *	Allocates page tables on demand.
 *
 *	If memory location already had a frame, checks to see if it has the
 *	same permissions. This is necessary since BSS and DATA occupy the same
 *	page-sized page-size aligned boundary. In fact BSS contiguously comes after
 *	DATA.
 *
 *
 *  @param pd Page table to allocate frame for
 *  @param virtual_address VM address to allocate frame for
 *  @param write_mode Permissions for the frame
 *  @param sys_prog_flag Our flags to determine if the page was allocated by
 *                       new_pages() or not
 *	@return 0 on success, -1 on error
 *
 */
static int
allocate_frame( uint32_t **pd, uint32_t virtual_address,
				write_mode_t write_mode, uint32_t sys_prog_flag )
{
	assert(write_mode == READ_WRITE || write_mode == READ_ONLY);
	if (!is_valid_sys_prog_flag(sys_prog_flag)) {
		return -1;
	}
	/* is_valid_pd() is expensive, hence the assert() */
	assert(is_valid_pd(pd));
	log("allocate frame for vm:%p", (uint32_t *) virtual_address);

	/* pd is NULL, calling thread has gone off the rails. Crash. */
	affirm(pd);

	/* Find page table entry corresponding to virtual address */
	uint32_t *ptep = get_ptep((const uint32_t **) pd, virtual_address);
	if (!ptep) {
		return -1;
	}
	uint32_t pt_entry = *ptep;

	/* If page table entry contains a non-NULL address */
	if (TABLE_ADDRESS(pt_entry)) {

		/* Must be present else broken invariant */
		affirm_msg(pt_entry & PRESENT_FLAG, "pt_entry must be present");

		/* Ensure it's allocated with same flags. */
		if (write_mode == READ_WRITE) {
			if ((pt_entry & (PAGE_SIZE - 1)) !=
				(PE_USER_WRITABLE | sys_prog_flag)) {
				return -1;
			}
		} else {
			if ((pt_entry & (PAGE_SIZE - 1)) !=
				(PE_USER_READABLE | sys_prog_flag)) {
				return -1;
			}
		}

	/* Page table entry contains a NULL address, allocate new physical frame */
	} else {
		uint32_t free_frame = physalloc();
		if (!free_frame) {
			return -1;
		}
		*ptep = free_frame;
	}

	if (write_mode == READ_WRITE) {
		*ptep |= (PE_USER_WRITABLE | sys_prog_flag);
	} else {
		*ptep |= (PE_USER_READABLE | sys_prog_flag);
	}

	return 0;
}

/** @brief Checks if system programmer flags are valid or not as defined in
 *         memory_manager_internal.h
 *
 *  @param sys_prog_flag System programmer flag
 *  @return 1 if valid flag, 0 otherwise
 */
int
is_valid_sys_prog_flag( uint32_t sys_prog_flag )
{
	switch (sys_prog_flag) {

		/* Empty sys_prog_flag is vacuously valid */
		case 0 :
			return 1;
			break;

		case NEW_PAGE_BASE_FLAG :
			return 1;
			break;

		case NEW_PAGE_CONTINUE_FROM_BASE_FLAG :
			return 1;
			break;

		/* Invalid sys programmer flag */
		default :
			return 0;
	}
}


/** @brief Allocates the system wide zero frame.
 *
 *  Currently this is only ever called by new_pages(), and thus
 *  allocated pages will always be writable by user programs. However since
 *  this is a zero frame, we set it to only have readonly access by the user.
 *
 *  When a user attempts to write to this physical frame, we check if the
 *  frame is the system wide zero frame and if it is, allocate it with
 *  user write permissions.
 *
 *  Requires that virtual address is valid
 *
 *  @param pd Page directory pointer
 *  @param virtual_address VM address we are allocating zero frame for
 *  @param sys_prog_flag Bits 9,10,11 to OR page table entry with
 *  @param 0 on success, -1 on error.
 */
int
allocate_user_zero_frame( uint32_t **pd, uint32_t virtual_address,
                          uint32_t sys_prog_flag )
{
	affirm(pd);

	/* is_valid_pd() is expensive, hence the assert() */
	assert(is_valid_pd(pd));

	/* For now only new_pages() calls this, so this assert is for debugging
	 * purposes only */
	assert(PAGE_ALIGNED(virtual_address));

	if (!is_valid_sys_prog_flag(sys_prog_flag)) {
		log_info("allocate_user_zero_frame(): "
				 "invalid sys_prog_flag:0x%x",
				 sys_prog_flag);
		return -1;
	}
	log("allocate_user_zero_frame(): "
	"allocate zero frame for vm:%p", (uint32_t *) virtual_address);

	/* Find page table entry corresponding to virtual address */
	uint32_t *ptep = get_ptep((const uint32_t **) pd, virtual_address);

	/* pd is valid, so !ptep means must add new page table to page directory */
	if (!ptep) {
		uint32_t pd_index = PD_INDEX(virtual_address);
		affirm(pd[pd_index] == NULL);
		add_new_pt_to_pd(pd, virtual_address);
		log_info("allocate_user_zero_frame(): "
		         "adding new pt to pd for virutal_address:0x%08lx",
				 virtual_address);
	}
	ptep = get_ptep((const uint32_t **) pd, virtual_address);
	affirm(ptep);
	uint32_t pt_entry = *ptep;

	/* If page table entry contains a non-NULL address */
	if (TABLE_ADDRESS(pt_entry)) {
		log_info("allocate_user_zero_frame(): "
				 "zero frame already allocated!");
		return -1;
	}
	/* Allocate new physical frame */
	*ptep = SYS_ZERO_FRAME;

	/* Mark as READ_ONLY for user */
	*ptep |= (sys_prog_flag | PE_USER_READABLE);

	invalidate_tlb((void *)virtual_address);

	return 0;
}

/** Allocates a memory region in virtual memory.
 *
 *	If there aren't enough physical frames to satisfy allocation
 *	request, region is not allocated and function returns a negative
 *	value.
 *
 *	@param pd	 Pointer to page directory
 *	@param start  Virtual memory addess for start of region to be allocated
 *	@param len	  Length of region to be allocated
 *	@param write_mode 0 if read-only region, non-zero value if writable
 *	@return 0 on success, negative value on failure.
 *	*/
static int
allocate_region( uint32_t **pd, void *start, uint32_t len,
                 write_mode_t write_mode )
{
	assert(write_mode == READ_WRITE || write_mode == READ_ONLY);
    uint32_t pages_to_alloc = (len + PAGE_SIZE - 1) / PAGE_SIZE;

    /* Ensure we have enough free frames to fulfill request */
    if (num_free_phys_frames() < pages_to_alloc) {
        return -1;
	}

	/* u_start is not always page-aligned. For example, the data segment is
	 * immediately succeeded by bss */
    uint32_t u_start = (uint32_t)start;

    /* Allocate 1 frame at a time. */
    for (int i = 0; i < pages_to_alloc; ++i) {
		uint32_t virtual_address = u_start + PAGE_SIZE * i;
		uint32_t pd_index = PD_INDEX(virtual_address);

		affirm((pd[pd_index] != NULL && TABLE_ADDRESS(pd[pd_index]) != 0)
		       || (pd[pd_index] == NULL));

		/* Get a new page table every time page directory entry is NULL */
		if (pd[pd_index] == NULL) {
			if (add_new_pt_to_pd(pd, virtual_address) < 0) {
				log_warn("allocate_region(): "
				         "unable to allocate new page table in pd:%p for "
						 "virtual_address: 0x%08lx", pd, virtual_address);
				return -1;
			}
		}
        int res = allocate_frame((uint32_t **)pd, virtual_address,
								 write_mode, 0);
		if (res < 0) {
			/* cleanup done by calling function new_pd_from_elf */
			return -1;
		}
	}
	return 0;
}


/** @brief Enables paging mechanism.
 *
 *  @return Void.
 */
static void
enable_paging( void )
{
	/* Enable paging flag */
	uint32_t current_cr0 = get_cr0();
	set_cr0(current_cr0 | PAGING_FLAG);

	affirm_msg(!paging_enabled, "Paging should be enabled exactly once!");
	paging_enabled = 1;
}

/** @brief Sets the page directory to cr3
 *
 *  @param pd Pointer to page directory
 *  @return Void.
 */
static void
vm_set_pd( void *pd )
{
	affirm(pd);
	uint32_t cr3 = get_cr3();
	/* Unset top 20 bits where new page table will be stored.*/
	cr3 &= PAGE_SIZE - 1;
	cr3 |= (uint32_t)pd;

	set_cr3(cr3);
}

/** @brief Frees a page table along with all physical frames
 *
 *	@param pt Page table to be freed
 *	@param pd_index Index of page table in page directory.
 *	@return Void.
 */
static void
free_pt_memory( uint32_t *pt, int pd_index )
{
    affirm(pt);
	assert(is_valid_pt(pt, pd_index));

	affirm( pd_index >= (USER_MEM_START >> PAGE_DIRECTORY_SHIFT));
	for (int i = 0; i < PAGE_SIZE / sizeof(uint32_t); ++i) {

		/* pt holds physical frames for user memory */
		if (pd_index >= (USER_MEM_START >> PAGE_DIRECTORY_SHIFT)) {
			uint32_t pt_entry = pt[i];
			if (pt_entry) {
				affirm(pt_entry & PRESENT_FLAG);
				affirm_msg(TABLE_ADDRESS(pt_entry) != 0, "pt_entry:0x%08lx",
						   pt_entry);

				/* Free only if not sys wide zero frame */
				uint32_t phys_address = TABLE_ADDRESS(pt_entry);
				if (phys_address != SYS_ZERO_FRAME)
					physfree(phys_address);

				/* always Zero the entry as well */
				pt[i] = 0;
			}
		}
	}
}

/** @brief Walks the page directory and frees the entire page directory,
 *		  page tables, and all physical frames
 *
 *  @param pd Page directory to be freed.
 */
void
free_pd_memory( void *pd )
{
	affirm(pd);
	assert(is_valid_pd(pd));
	uint32_t **pd_cast = (uint32_t **) pd;

	for (int i = NUM_KERN_PAGE_TABLES; i < PAGE_SIZE / sizeof(uint32_t); ++i) {

		uint32_t *pd_entry = pd_cast[i];

		/* Check page table if entry non-zero */
		if ((uint32_t) pd_entry) {
			affirm(((uint32_t) pd_entry) & PRESENT_FLAG);
			uint32_t *pt = (uint32_t *) TABLE_ADDRESS(pd_entry);
			free_pt_memory(pt, i);
			sfree(pt, PAGE_SIZE);
		}
	}
}

