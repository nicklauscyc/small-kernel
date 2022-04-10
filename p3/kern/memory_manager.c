/** @file memory_manager.c
 *  @brief Functions to initialize and manage virtual memory
 *
 *
 *
 *  TODO need to figure out when to free physical pages, probably when
 *  cleaning up thread resources ?
 *
 *  @author Andre Nascimento (anascime)
 *  @author Nicklaus Choo (nchoo)
 *
 * */

#include <physalloc.h> /* physalloc() */
#include <memory_manager.h>
#include <stdint.h>     /* uint32_t */
#include <stddef.h>     /* NULL */
#include <malloc.h>     /* smemalign, sfree */
#include <elf_410.h>    /* simple_elf_t */
#include <assert.h>     /* assert, affirm */
#include <page.h>       /* PAGE_SIZE */
#include <x86/cr.h>     /* {get,set}_{cr0,cr3} */
#include <string.h>     /* memset, memcpy */
#include <common_kern.h>/* USER_MEM_START */
#include <logger.h>     /* log */

#define PAGING_FLAG (1 << 31)
#define WRITE_PROTECT_FLAG (1 << 16)

#define PAGE_DIRECTORY_INDEX 0xFFC00000
#define PAGE_TABLE_INDEX 0x003FF000
#define PAGE_OFFSET 0x00000FFF

#define PAGE_DIRECTORY_SHIFT 22
#define PAGE_TABLE_SHIFT 12

/* Get page directory or page table index from a logical/linear address. */
#define PD_INDEX(addr) \
    ((PAGE_DIRECTORY_INDEX & (addr)) >> PAGE_DIRECTORY_SHIFT)
#define PT_INDEX(addr) \
    ((PAGE_TABLE_INDEX & (addr)) >> PAGE_TABLE_SHIFT)

/* Flags for page directory and page table entries */
#define PRESENT_FLAG 1 << 0
#define RW_FLAG      1 << 1
#define US_FLAG      1 << 2
#define GLOBAL_FLAG  1 << 8

#define PE_USER_READABLE (PRESENT_FLAG | US_FLAG )
#define PE_USER_WRITABLE (PE_USER_READABLE | RW_FLAG)

/* Set global flag so TLB doesn't flush kernel entries */
#define PE_KERN_READABLE (PRESENT_FLAG | GLOBAL_FLAG | RW_FLAG)
#define PE_KERN_WRITABLE (PE_KERN_READABLE | RW_FLAG)
#define PE_UNMAPPED 0

#define PT_ADDRESS(PD_ENTRY) (((uint32_t) (PD_ENTRY)) & ~(PAGE_SIZE - 1))
/* 1 if VM is enabled, 0 otherwise */
static int vm_enabled = 0;

static uint32_t *get_ptep( uint32_t **pd, uint32_t virtual_address );
static int allocate_frame( uint32_t **pd,
        uint32_t virtual_address, write_mode_t write_mode );
static int allocate_region( void *pd, void *start,
        uint32_t len, write_mode_t write_mode );
static void enable_paging( void );
static int valid_memory_regions( simple_elf_t *elf );
static void vm_set_pd( void *pd );
static void free_pt_memory( void *pt );

/** @brief Sets up a new page directory by allocating physical memory for it.
 *		   Does not transfer executable data into physical memory.
 *
 *  Assumes page table directory is empty. Sets appropriate
 *  read/write permissions. To copy memory over, set the WP flag
 *  in the CR0 register to 0 - this will make it so that write
 *  protection is ignored by the paging mechanism.
 *
 *  Allocated pages are initialized to 0.
 *
 *  TODO: Implement ZFOD here. (Handler should probably be defined
 *  elsewhere, though)
 *
 *  @return Page directory that is backed by physical memory
 *  */
void *
new_pd_from_elf( simple_elf_t *elf, uint32_t stack_lo, uint32_t stack_len )
{
    void *pd = smemalign(PAGE_SIZE, PAGE_SIZE);
    if (!pd) {
        return NULL;
	}
    /* Ensure all entries are 0 and therefore not present */
    memset(pd, 0, PAGE_SIZE);
	assert(PAGE_ALIGNED((uint32_t) pd));

    /* Direct map all 16MB for kernel, setting correct permission bits */
    for (uint32_t addr = 0; addr < USER_MEM_START; addr += PAGE_SIZE) {
        uint32_t *ptep = get_ptep(pd, addr);
		if (!ptep) {
            sfree(pd, PAGE_SIZE);
			return NULL;
		}
		/* Indicate page table entry permissions */
		if (addr == 0) {
            *ptep = addr | PE_UNMAPPED; /* Leave NULL unmapped. */
        } else {
            *ptep = addr | PE_KERN_WRITABLE; /* PE_KERN_WRITABLE FIXME */
        }
    }
    /* Allocate regions with appropriate read/write permissions.
     * TODO: Free allocated regions if later allocation fails. */
    int i = 0;

	// TODO: implement valid_memory_regions
    if (!valid_memory_regions(elf)) {
        sfree(pd, PAGE_SIZE);
        return NULL;
    }

    i += allocate_region(pd, (void *)elf->e_txtstart, elf->e_txtlen, READ_ONLY);
    i += allocate_region(pd, (void *)elf->e_datstart, elf->e_datlen, READ_WRITE);
    i += allocate_region(pd, (void *)elf->e_rodatstart, elf->e_rodatlen, READ_ONLY);
    i += allocate_region(pd, (void *)elf->e_bssstart, elf->e_bsslen, READ_WRITE);
    i += allocate_region(pd, (void *)stack_lo, stack_len, READ_WRITE);

    if (i < 0) {
        sfree(pd, PAGE_SIZE);
        return NULL;
    }

    return pd;
}

/** @brief Initialized child pd from parent pd. Deep copies writable
 *         entries, allocating new physical frame. Returns child_pd on success
 *
 *         TODO: Revisit to implement ZFOD. Also, avoid flushing tlb
 *         all the time.
 * */
void *
new_pd_from_parent( void *v_parent_pd )
{
    uint32_t *parent_pd = (uint32_t *)v_parent_pd;
    /* Create temp_buf for deep copy. Too large to stack allocate */
    uint32_t *child_pd = smemalign(PAGE_SIZE, PAGE_SIZE);
    if (!child_pd) {
        return NULL;
    }
	assert(PAGE_ALIGNED(child_pd));
    memset(child_pd, 0, PAGE_SIZE); /* Set all entries to empty initially */

    uint32_t *temp_buf = smemalign(PAGE_SIZE, PAGE_SIZE);
    if (!temp_buf) {
        sfree(child_pd, PAGE_SIZE);
        return NULL;
    }

    for (int i=0; i < (PAGE_SIZE / sizeof(uint32_t)); ++i) {

        if ((parent_pd[i] & PRESENT_FLAG) && (parent_pd[i] & RW_FLAG)) {
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

			// update child_pd[i]
			child_pd[i] = (uint32_t) child_pt;
			// OR the flags
			child_pd[i] |= (parent_pd[i] & (PAGE_SIZE - 1));

			// Get address of parent_pt
            uint32_t *parent_pt = (uint32_t *)(parent_pd[i] & ~(PAGE_SIZE - 1));
            assert(PAGE_ALIGNED(parent_pt));

			// parent_pt and child_pt are actual addresses without flags

            /* Copy entries in page tables */
            for (int j=0; j < (PAGE_SIZE / sizeof(uint32_t)); ++j) {

				// Constructing the virtual address
				uint32_t vm_address = ((i << 22) | (j << 12));
				assert(PAGE_ALIGNED(vm_address));

				// direct map kernel memory
				if (vm_address < USER_MEM_START) {
					child_pt[j] = parent_pt[j];
					continue;
				}
                if ((parent_pt[j] & PRESENT_FLAG) && (parent_pt[j] & RW_FLAG)) {
                    /* Allocate new physical frame for child. */
                    child_pt[j] = physalloc();
					assert(PAGE_ALIGNED(child_pt[j]));
                    if (!child_pt[j]) {
                        free_pd_memory(child_pd); // Cleanup previous allocs

                        sfree(temp_buf, PAGE_SIZE);
                        sfree(child_pd, PAGE_SIZE);
                        return NULL;
                    }

                    /* Set child_pt flags, then memcpy */
                    child_pt[j] |= parent_pt[j] & (PAGE_SIZE - 1);

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

                } else {
                    child_pt[j] = parent_pt[j];
                }
            }

        } else {
            child_pd[i] = parent_pd[i];
        }
    }

    sfree(temp_buf, PAGE_SIZE);
    return child_pd;
}

/** @brief Sets new page table directory and enables paging if necessary.
 *
 *  Paging should only be set once and never disabled.
 *
 *  @param pd Page directory pointer
 *  @return Void.
 */
void
vm_enable_task( void *pd )
{
	affirm_msg(pd, "Page directory must be non-NULL!");
	affirm_msg(PAGE_ALIGNED(pd), "Page directory must be page aligned!");
	affirm_msg((uint32_t) pd < USER_MEM_START,
			   "Page directory must in kernel memory!");

    vm_set_pd(pd);
	if (!vm_enabled) {
		enable_paging();
	}
}

/** @brief Enables write_protect flag in cr0, allowing
 *  kernel to bypass VM's read-only protection. */
void
enable_write_protection( void )
{
	uint32_t current_cr0 = get_cr0();
	set_cr0(current_cr0 | WRITE_PROTECT_FLAG);
}

/** @brief Disables write_protect flag in cr0, stopping
 *  kernel from bypassing VM's read-only protection. */
void
disable_write_protection( void )
{
	uint32_t current_cr0 = get_cr0();
	set_cr0(current_cr0 & (~WRITE_PROTECT_FLAG));
}

/** Allocate new pages in a given process' virtual memory. */
int
vm_new_pages ( void *pd, void *base, int len )
{
    // TODO: Implement
    (void)pd;
    (void)base;
    (void)len;
    return -1;
}

/** @brief Checks if a user pointer is valid.
 *	Valid means the pointer is non-NULL, belongs to
 *	user memory and is in an allocated memory region.
 *
 *	@param ptr Pointer to check
 *	@param read_write Whether to check for write permission
 *	@return 1 if valid, 0 if not */
int
is_valid_user_pointer(void *ptr, write_mode_t write_mode)
{
	/* Check not kern memory and non-NULL */
	if ((uint32_t)ptr < USER_MEM_START)
		return 0;

	/* Check if allocated */
	uint32_t *pd = (uint32_t *)(get_cr3() & ~(PAGE_SIZE - 1));
	if (!(pd[PD_INDEX((uint32_t)ptr)] & PRESENT_FLAG))
		return 0;
	uint32_t *pt = (uint32_t *)(pd[PD_INDEX((uint32_t)ptr)] & ~(PAGE_SIZE - 1));
	if (!(pt[PT_INDEX((uint32_t)ptr)] & PRESENT_FLAG))
		return 0;

	if (write_mode == READ_WRITE && !(pt[PT_INDEX((uint32_t)ptr)] & RW_FLAG))
		return 0;

	return 1;
}


/** @brief Checks that the address of every character in the string is a valid
 *		   address. Does not check for write permissions!
 *
 *	@param s String to be checked
 *	@param len Length of string
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

		if (!is_valid_user_pointer(s + i, READ_ONLY)) {
			log_warn("invalid address %p at index %d of user string %s",
					 s + i, i, s);
			return 0;

		} else {

			/* String has ended within USER_STR_LEN */
			if (s[i] == '\0') {
				break;
			}
		}
	}
	/* Check length of s */
	if (i == len && null_terminated) {
		log_warn("user string of length >= USER_STR_LEN");
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
 *	@param len Length of string
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
 *	@param len Length of string
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
is_valid_user_argvec( char *execname,  char **argvec )
{
	/* Check address of every char * in argvec */
	int i;
	for (i = 0; i < NUM_USER_ARGS; ++i) {

		/* Invalid char ** */
		if (!is_valid_user_pointer(argvec + i, READ_ONLY)) {
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
 *  @return Pointer in kernel VM of page table if successful, 0 otherwise
 */
void *
get_new_pt( void )
{
	/* Allocate memory for a new page table */
	void *pt = smemalign(PAGE_SIZE, PAGE_SIZE);
	if (!pt) {
		return 0;
	}
	log("new pt at address %p", pt);
	assert(PAGE_ALIGNED((uint32_t) pt));

	/* Initialize all page table entries as non-present */
	memset(pt, 0, PAGE_SIZE);

	return pt;
}

/** @brief Gets pointer to page table entry in a given page directory.
 *         Allocates page table if necessary.
 *
 *  @param pd Page table address
 *  @param virtual_address Virtual address corresponding to page table
 *  @return Pointer to a page table entry that can be dereferenced to get
 *          a physical address
 */
static uint32_t *
get_ptep( uint32_t **pd, uint32_t virtual_address )
{
    affirm(pd);
    uint32_t pd_index = PD_INDEX(virtual_address);
    uint32_t pt_index = PT_INDEX(virtual_address);

	/* If page table has not been allocated, allocate a new one */
    if (!((uint32_t)pd[pd_index] & PRESENT_FLAG)) {
		void *pt = get_new_pt();
		if (!pt) {
			return 0;
		}
		/* Page table should be in kernel memory */
		assert((uint32_t) pt < USER_MEM_START);
        pd[pd_index] = pt;

        /* Set all page directory entries as writable, determine
         * whether truly writable in page table entry. */
        pd[pd_index] = (uint32_t *)((uint32_t)pd[pd_index] | PE_USER_WRITABLE);
    }
    /* Page table entry pointer zeroes out bottom 12 bits */
    uint32_t *ptep = (uint32_t *)((uint32_t)pd[pd_index] & ~(PAGE_SIZE - 1));

	/* Return pointer to appropriate index */
	return ptep + pt_index;
}

/** Allocate new frame at given virtual memory address.
 *  Allocates page tables on demand.
 *
 *  If memory location already had a frame, checks whether it's allocated with
 *  the same flags as this function would set.
 *  */
static int
allocate_frame( uint32_t **pd, uint32_t virtual_address, write_mode_t write_mode )
{
    affirm(pd);

    uint32_t *pte = get_ptep(pd, virtual_address);

    if ((*pte & PRESENT_FLAG) != 0) { /* if allocated */
        /* Ensure it's allocated with same flags. */
        if (write_mode == READ_WRITE)
            affirm((*pte & (PAGE_SIZE - 1)) == PE_USER_WRITABLE);
        else
            affirm((*pte & (PAGE_SIZE - 1)) == PE_USER_READABLE);

        return 0;
    }
    uint32_t free_frame = physalloc();
	if (!free_frame) {
		return -1;
	}

    *pte = free_frame;

    /* FIXME: Hack for until we implement ZFOD. Do we even want to guarantee
     * zero-filled pages for the initially allocated regions? Seems like
     * .bss and new_pages are the only ones required to be zeroed out by spec.
     * Should we even be calling enable paging here??? */
    /* ATOMICALLY start */
    //disable_paging();
    //memset((void *)free_frame, 0, PAGE_SIZE);
    //enable_paging();
    /* ATOMICALLY end*/

    if (write_mode == READ_WRITE) {
        *pte |= PE_USER_WRITABLE;
    } else {
        *pte |= PE_USER_READABLE;
	}

	return 0;
}

/** Allocates a memory region in virtual memory.
 *
 *  If there aren't enough physical frames to satisfy allocation
 *  request, region is not allocated and function returns a negative
 *  value.
 *
 *  @param pd    Pointer to page directory
 *  @param start  Virtual memory addess for start of region to be allocated
 *  @param len    Length of region to be allocated
 *  @param write_mode 0 if read-only region, non-zero value if writable
 *
 *  @return 0 on success, negative value on failure.
 *  */
static int
allocate_region( void *pd, void *start, uint32_t len, write_mode_t write_mode )
{
    uint32_t pages_to_alloc = (len + PAGE_SIZE - 1) / PAGE_SIZE;

    /* Ensure we have enough free frames to fulfill request */
    if (num_free_phys_frames() < pages_to_alloc) {
        return -1;
	}
    /* FIXME: Do we have any guarantee memory regions are page aligned?
     *        They should be, to some extent. At the very least, 2 memory
     *        regions should not be intersect with the same page, as they
     *        could require distinct permissions. THis might not be the case
     *        for data and bss, though, as both are read-write sections. */
    uint32_t u_start = (uint32_t)start;
    /* Allocate 1 frame at a time. */
    for (int i = 0; i < pages_to_alloc; ++i) {
        int res = allocate_frame((uint32_t **)pd, u_start + PAGE_SIZE * i,
								 write_mode);
		if (res < 0) {
			// TODO CLEAN UP ALL PREVIOUSLY ALLOCATED PHYS FRAMES
			return -1;
		}
	}
    return 0;
}

static int
valid_memory_regions( simple_elf_t *elf )
{
    /* TODO:
     * - Check if memory regions intersect each other. If so, false.
     * - Check if memory regions intersect same page. If so and they
     *   have different read/write permissions, false.
     * - Othws, true
     *  */
    return 1;
}

/** @brief Enables paging mechanism. */
static void
enable_paging( void )
{
	uint32_t current_cr0 = get_cr0();
	set_cr0(current_cr0 | PAGING_FLAG);
	affirm_msg(!vm_enabled, "Paging should be enabled exactly once!");
	vm_enabled = 1;
}

static void
vm_set_pd( void *pd )
{
    uint32_t cr3 = get_cr3();
    /* Unset top 20 bits where new page table will be stored.*/
    cr3 &= PAGE_SIZE - 1;
    cr3 |= (uint32_t)pd;

    set_cr3(cr3);
}

/** brief Walks the page directory and frees the entire page directory,
 *        page tables, and all physical frames
 */
void
free_pd_memory( void *pd )
{
	//TODO this needs to be debugged for bs page table entries
	return;
	affirm(is_valid_pd(pd));
	uint32_t *pd_to_free = (uint32_t *) pd;
	int num_pd_entries = PAGE_SIZE / sizeof(uint32_t);
	for (int i = 0; i < num_pd_entries; ++i) {

		/* Free page table if entry non-zero */
		if (pd_to_free[i]) {
			log("page directory entry at index %d:0x%08lx", i, pd_to_free[i]);
			uint32_t pt = PT_ADDRESS(pd_to_free[i]);
			log("freeing page table at address %p", pt);
			free_pt_memory((uint32_t *) pt);
			sfree((uint32_t *) pt, PAGE_SIZE);
		}
	}
}

/** @brief Frees a page table along with all physical frames
 *
 *  @param pt Page table to be freed
 */
static void
free_pt_memory( void * pt) {
	affirm(is_valid_pd(pt));
	uint32_t *pt_to_free = (uint32_t *) pt;
	int num_pt_entries = PAGE_SIZE / sizeof(uint32_t);
	for (int i = 0; i < num_pt_entries; ++i) {

		/* Free physical frame */
		if (pt_to_free[i] >= USER_MEM_START) {
			uint32_t physframe = PT_ADDRESS(pt_to_free[i]);
			log("freeing physical address:0x%lx", physframe);
			physfree(physframe);
		}
	}
}

/** @brief Checks if supplied page directory is valid or not
 *
 *  TODO keep a record of all page directory addresses ever given out?
 *
 *  @param pd Page directory to check
 */
int
is_valid_pd( void *pd )
{
	return pd && ((uint32_t) pd < USER_MEM_START) && PAGE_ALIGNED(pd);
}
