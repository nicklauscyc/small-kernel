/** @file memory_manager.c
 *  @brief Functions to initialize and manage virtual memory
 *
 *  Note that page directory and page table consistency/validity checks are
 *  expensive and thus carried out in assertions.
 *
 *  TODO need to figure out when to free physical pages, probably when
 *  cleaning up thread resources ?
 *
 *  @author Andre Nascimento (anascime)
 *  @author Nicklaus Choo (nchoo)
 *
 * */

#include <simics.h>
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

#define PAGE_GLOBAL_ENABLE_FLAG (1 << 7)

#define PAGE_DIRECTORY_INDEX 0xFFC00000
#define PAGE_TABLE_INDEX 0x003FF000
#define PAGE_OFFSET 0x00000FFF

#define PAGE_DIRECTORY_SHIFT 22
#define PAGE_TABLE_SHIFT 12

#define TABLE_ENTRY_INVARIANT(TABLE_ENTRY)\
    ((((TABLE_ENTRY) != NULL) && (TABLE_ADDRESS(TABLE_ENTRY) != 0))\
    || ((TABLE_ENTRY) == NULL))

/* Get page directory index from logical address */
#define PD_INDEX(addr) \
    ((PAGE_DIRECTORY_INDEX & ((uint32_t)(addr))) >> PAGE_DIRECTORY_SHIFT)

/* Get page table index from logical address */
#define PT_INDEX(addr) \
    ((PAGE_TABLE_INDEX & ((uint32_t)(addr))) >> PAGE_TABLE_SHIFT)

/* Flags for page directory and page table entries */
#define PRESENT_FLAG (1 << 0)
#define RW_FLAG      (1 << 1)
#define USER_FLAG    (1 << 2)
#define GLOBAL_FLAG  (1 << 8)

#define PE_USER_READABLE (PRESENT_FLAG | USER_FLAG )
#define PE_USER_WRITABLE (PE_USER_READABLE | RW_FLAG)

/* Set global flag so TLB doesn't flush kernel entries */
#define PE_KERN_READABLE (PRESENT_FLAG | GLOBAL_FLAG | RW_FLAG)
#define PE_KERN_WRITABLE (PE_KERN_READABLE | RW_FLAG)
#define PE_UNMAPPED 0

/* 1 if VM is enabled, 0 otherwise */
static int vm_enabled = 0;
static uint32_t sys_zero_frame = USER_MEM_START;
static int vm_init = 0;

/* [USER_MEM_START, USER_MEM_START + PAGE_SIZE) is all zeroes */
//static uint32_t system_zero_frame = USER_MEM_START;


static uint32_t *get_ptep( const uint32_t **pd, uint32_t virtual_address );
static int allocate_frame( uint32_t **pd, uint32_t virtual_address,
                          write_mode_t write_mode );
static int allocate_region( uint32_t **pd, void *start, uint32_t len,
                           write_mode_t write_mode );
static void enable_paging( void );
static int valid_memory_regions( simple_elf_t *elf );
static void vm_set_pd( void *pd );
static void free_pt_memory( uint32_t *pt, int pd_index );

/* TODO do we need this? */
uint32_t vm_address_from_index( uint32_t pd_index, uint32_t pt_index );
static int is_valid_pt( uint32_t *pt, int pd_index );
static void *allocate_new_pd( void );
static int add_new_pt_to_pd( uint32_t **pd, uint32_t virtual_address );

/** @brief Returns pointer to page directory from cr3(), guarantees that pointer
 *         is non-NULL and page aligned, and below USER_MEM_START
 *
 *  Does consistency check for valid page directory if NDEBUG is not defined.
 *
 *  @return Pointer to current active page directory
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

int
zero_page_pf_handler( uint32_t faulting_address )
{
    log("zero_page_pf_handler() called");
    /* get_pd() guarantees basic consistency for valid page directory */
    uint32_t **pd = get_pd();

    //TODO a version of get_ptep that does not allocate the pagetable at l
    uint32_t *ptep = get_ptep( (const uint32_t **) pd, faulting_address);

    /* Page table entry cannot be NULL frame */
    if (!ptep) {
        log_warn("page table entry for vm 0x%08lx is NULL!",
                 faulting_address);
        return -1;
    }
    uint32_t pt_entry = *ptep;

    /* Page table entry must hold the system wide zero frame */
    if (TABLE_ADDRESS(pt_entry) != sys_zero_frame) {
        log_warn("page table entry for vm 0x%08lx is not zero frame",
                 faulting_address);
        MAGIC_BREAK;
        return -1;
    }
    /* Page table entry must be user readable since sys wide zero frame */
    assert((pt_entry & PE_USER_READABLE) == PE_USER_READABLE);

    /* Unallocate zero frame */
    unallocate_user_zero_frame(pd, faulting_address);

    /* Back up with actual frame */
    // TODO version of allocate_frame that throws an error if already allocated
    int res = allocate_frame(pd, faulting_address, READ_WRITE);
    log("alloced physframe is 0x%08lx", *get_ptep((const uint32_t **) pd,
                                                  faulting_address));
    log("res:%d", res);
    MAGIC_BREAK;
    return res;
}

void
init_vm( void )
{
    affirm(!vm_init);
    affirm(!vm_enabled);
    vm_init = 1;

    /* Zero fill system wide zero frame */
    memset((uint32_t *) sys_zero_frame, 0, PAGE_SIZE);
}

/** @brief Sets up a new page directory by allocating physical memory for it.
 *         Does not transfer executable data into physical memory.
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
    /* If not initialized, initialize vm */
    if (!vm_init) {
        init_vm();
    }
    /* Allocate new pd that is zero filled */
    uint32_t **pd = allocate_new_pd();
    if (!pd) {
        log_warn("new_pd_from_elf(): "
                 "unable to allocate new page directory.");
        return NULL;
    }
    /* Direct map all 16MB for kernel, setting correct permission bits */
    for (uint32_t addr = 0; addr < USER_MEM_START; addr += PAGE_SIZE) {

        /* This invariant must not break */
        uint32_t pd_index = PD_INDEX(addr);
        uint32_t *pd_entry = pd[pd_index];
        affirm_msg(TABLE_ENTRY_INVARIANT(pd_entry),
                   "new_pd_from_elf(): "
                   "pd entry invariant broken for "
                   "pd:%p pd_index:0x%08lx pd[pd_index]:%p",
                   pd, pd_index, pd_entry);

        /* Add new page table every time page directory entry is NULL */
        if (pd[pd_index] == NULL) {

            /* Since we are going in increasing virtual addresses, this holds */
            assert((addr & ((1 << PAGE_DIRECTORY_SHIFT) - 1)) == 0);
            if (add_new_pt_to_pd(pd, addr) < 0) {
                log_warn("new_pd_from_elf(): "
                         "unable to allocate new page table in pd:%p for "
                         "virtual_address: 0x%08lx", pd, addr);
                return NULL;
                //TODO clean up
            }
        }
        /* Now get a pointer to the corresponding page table entry */
        uint32_t *ptep = get_ptep((const uint32_t **) pd, addr);

        /* If NULL is returned, free all resources in page directory */
        if (!ptep) {
            free_pd_memory(pd);
            sfree(pd, PAGE_SIZE);
            log_warn("new_pd_from_elf(): "
                     "unable to get page table entry pointer.");
            return NULL;
        }
        /* Indicate page table entry permissions */
        if (addr == 0) {
            *ptep = addr | PE_UNMAPPED; /* Leave NULL unmapped. */
        } else {
            *ptep = addr | PE_KERN_WRITABLE; /* Can delete this?: PE_KERN_WRITABLE FIXME */
        }
        assert(*ptep < USER_MEM_START);
    }
    log("new_pd_from_elf(): direct map ended");
    /* Allocate regions with appropriate read/write permissions.
     * TODO: Free allocated regions if later allocation fails. */
    int i = 0;

    // TODO: implement valid_memory_regions
    if (!valid_memory_regions(elf)) {
        free_pd_memory(pd);
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

    assert(is_valid_pd(pd));
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

        /* Set PGE flag in cr4 so kernel mappings not flushed on context switch */
        set_cr4(CR4_PGE | get_cr4());
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
 *  Valid means the pointer is non-NULL, belongs to
 *  user memory and is in an allocated memory region.
 *
 *  @param ptr Pointer to check
 *  @param read_write Whether to check for write permission
 *  @return 1 if valid, 0 if not */
int
is_valid_user_pointer(void *ptr, write_mode_t write_mode)
{
    /* Check not kern memory and non-NULL */
    if ((uint32_t)ptr < USER_MEM_START)
        return 0;

    /* Check if allocated */
    if (!is_user_pointer_allocated(ptr)) {
        return 0;
    }
    /* Check for correct write_mode */
    uint32_t **pd = (uint32_t **)TABLE_ADDRESS(get_cr3());
    uint32_t pd_index = PD_INDEX(ptr);
    uint32_t pt_index = PT_INDEX(ptr);
    uint32_t *pt = (uint32_t *) TABLE_ADDRESS(pd[pd_index]);
    if (write_mode == READ_WRITE && !(pt[pt_index] & RW_FLAG))
        return 0;


    return 1;
}

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
 *         address
 *
 *  //TODO test this function and see if it catches stuff
 *
 *  The maximum permitted string length is USER_STR_LEN, including '\0'
 *  terminating character. Therefore the longest possible user string will
 *  have at most USER_STR_LEN - 1 non-NULL characters.
 *
 *  This does not check for the existence of a user executable with this
 *  name. That is done when we try to fill in the ELF header.
 *         address. Does not check for write permissions!
 *
 *  @param s String to be checked
 *  @param len Length of string
 *  @param null_terminated  Whether the string should be checked
 *                          for null-termination
 *  @return 1 if valid user string, 0 otherwise
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
 *         address. Does not check for write permissions!
 *
 *  The maximum permitted string length is max_len, including '\0'
 *  terminating character. Therefore the longest possible user string will
 *  have at most max_len - 1 non-NULL characters.
 *
 *  This does not check for the existence of a user executable with this
 *  name. That is done when we try to fill in the ELF header.
 *
 *  @param s String to be checked
 *  @param len Length of string
 *  @return 1 if valid user string, 0 otherwise
 */
int
is_valid_null_terminated_user_string( char *s, int len )
{
    return is_valid_user_string_helper(s, len, 1);
}

/** @brief Checks that the address of every character in the string is a valid
 *         address. Does not check for write permissions!
 *
 *  @param s String to be checked
 *  @param len Length of string
 *  @return 1 if valid user string, 0 otherwise
 */
int
is_valid_user_string( char *s, int len )
{
    return is_valid_user_string_helper(s, len, 0);
}

/** @brief Checks address of every char * in argvec, argvec has max length
 *         of < NUM_USER_ARGS
 *
 *  //TODO test this function and see if it catches stuff
 *
 *  @param execname Executable name
 *  @param argvec Argument vector
 *  @return Number of user args if valid argvec, 0 otherwise
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
 *  Ensures that the allocated page table has an address that is page
 *  aligned
 *
 *  @return Pointer in kernel VM of page table if successful, 0 otherwise
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
    assert(PAGE_ALIGNED((uint32_t) pt));

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
    assert(PAGE_ALIGNED((uint32_t) pd));

    /* Initialize all page table entries as non-present */
    memset(pd, 0, PAGE_SIZE);

    return pd;
}

//done
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
 *         Allocates page table if necessary.
 *
 *  @param pd Page table address
 *  @param virtual_address Virtual address corresponding to page table
 *  @return Pointer to a page table entry that can be dereferenced to get
 *          a physical address, NULL on failure.
 */
static uint32_t *
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
    /* Page table must have correct flag bits set */
    if (((uint32_t) pd[pd_index] & PE_USER_WRITABLE) != PE_USER_WRITABLE) {
        log_warn("get_ptep(): "
                 "pd[pd_index]:%p does not have PE_USER_WRITABLE bits set!",
                 pd[pd_index]);
        return NULL;
    }
    /* Page table entry pointer zeroes out bottom 12 bits */
    uint32_t *ptep = (uint32_t *) TABLE_ADDRESS(pd[pd_index]);

    /* Return pointer to appropriate index */
    ptep += pt_index;
    affirm_msg(STACK_ALIGNED(ptep), "ptep:%p not stack aligned!", ptep);
    return ptep;
}

/** @brief Allocate new frame at given virtual memory address.
 *  Allocates page tables on demand.
 *
 *  If memory location already had a frame, checks to see if it has the
 *  same permissions. This is necessary since BSS and DATA occupy the same
 *  page-sized page-size aligned boundary. In fact BSS contiguously comes after
 *  DATA.
 *
 *  TODO CR4 needs to be set to prevent TLB flush
 *
 *  @return 0 on success, -1 on error
 *  */
static int
allocate_frame( uint32_t **pd, uint32_t virtual_address,
                write_mode_t write_mode )
{
    /* is_valid_pd() is expensive, hence the assert() */
    assert(is_valid_pd(pd));
    log("allocate frame for vm:%p", (uint32_t *) virtual_address);

    /* pd is NULL, abort with error */
    if (!pd) {
        return -1;
    }
    /* Find page table entry corresponding to virtual address */
    uint32_t *ptep = get_ptep((const uint32_t **) pd, virtual_address);
    if (!ptep) {
        return -1;
    }
    uint32_t pt_entry = *ptep;

    /* If page table entry contains a non-NULL address */
    if (TABLE_ADDRESS(pt_entry)) {
        log_warn("frame already allocated for vm:%p", (uint32_t *) virtual_address);
        //return -1;

        /* Must be present else broken invariant */
        affirm_msg(pt_entry & PRESENT_FLAG, "pt_entry must be present");

        /* Ensure it's allocated with same flags. */
        if (write_mode == READ_WRITE) {
            if (!((pt_entry & (PAGE_SIZE - 1)) == PE_USER_WRITABLE)) {
                return -1;
            }
        } else {
            if (!((pt_entry & (PAGE_SIZE - 1)) == PE_USER_READABLE)) {
                return -1;
            }
        }
        return 0;

    /* Page table entry contains a NULL address, allocate new physical frame */
    } else {
        uint32_t free_frame = physalloc();
        if (!free_frame) {
            return -1;
        }
        *ptep = free_frame;
    }
    /* FIXME: Hack for until we implement ZFOD. Do we even want to guarantee
     * zero-filled pages for the initially allocated regions? Seems like
     * .bss and new_pages are the only ones required to be zeroed out by spec.
     * Should we even be calling enable paging here??? */
    /* ATOMICALLY start */
    /* ATOMICALLY end*/
    if (write_mode == READ_WRITE) {
        *ptep |= PE_USER_WRITABLE;
    } else {
        *ptep |= PE_USER_READABLE;
    }
    return 0;
}

/** @brief Allocates the system wide zero frame
 */
int
allocate_user_zero_frame( uint32_t **pd, uint32_t virtual_address)
{
    /* is_valid_pd() is expensive, hence the assert() */
    assert(is_valid_pd(pd));
    log("allocate zero frame for vm:%p", (uint32_t *) virtual_address);

    /* pd is NULL, abort with error */
    if (!pd) {
        log_warn("allocate_zero_frame pd cannot be NULL!");
        return -1;
    }
    /* Find page table entry corresponding to virtual address */
    uint32_t *ptep = get_ptep((const uint32_t **) pd, virtual_address);
    if (!ptep) {
        log_warn("unable to get page table entry");
        return -1;
    }
    uint32_t pt_entry = *ptep;

    /* If page table entry contains a non-NULL address */
    if (TABLE_ADDRESS(pt_entry)) {
        log_warn("zero frame already allocated!");
        return -1;
    }
    /* Allocate new physical frame */
    *ptep = sys_zero_frame;

    /* Mark as READ_ONLY for user */
    *ptep |= PE_USER_READABLE;
    return 0;
}

void
unallocate_user_zero_frame( uint32_t **pd, uint32_t virtual_address)
{
    /* is_valid_pd() is expensive, hence the assert() */
    assert(is_valid_pd(pd));
    log("unallocate zero frame for vm:%p", (uint32_t *) virtual_address);

    /* pd is NULL, abort with error */
    affirm_msg(pd, "unallocate_zero_frame pd cannot be NULL!");

    /* Find page table entry corresponding to virtual address */
    uint32_t *ptep = get_ptep((const uint32_t **) pd, virtual_address);
    affirm_msg(ptep, "unable to get page table entry");

    uint32_t pt_entry = *ptep;

    /* If page table entry contains a non-NULL address */
    affirm_msg(TABLE_ADDRESS(pt_entry) == sys_zero_frame,
               "should be a zero frame allocated here!");

    /* zero frame should be marked as READ_ONLY for users */
    affirm_msg((pt_entry & PE_USER_READABLE) == PE_USER_READABLE,
               "zero frame should be PE_USER_READABLE");

    /* Unallocate new physical frame */
    *ptep = 0;
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
allocate_region( uint32_t **pd, void *start, uint32_t len, write_mode_t write_mode )
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
    /* Enable paging flag */
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



/** @brief Frees a page table along with all physical frames
 *
 *
 *  //TODO do we free the page tables less than USER_MEM_START or do we
 *  keep them global for aliasing?
 *
 *  @param pt Page table to be freed
 */
static void
free_pt_memory( uint32_t *pt, int pd_index ) {

    affirm(is_valid_pt(pt, pd_index));

    /* pt holds physical frames for user memory */
    if (pd_index >= (USER_MEM_START >> PAGE_DIRECTORY_SHIFT)) {
        for (int i = 0; i < PAGE_SIZE / sizeof(uint32_t); ++i) {

            uint32_t pt_entry = pt[i];
            if (pt_entry & PRESENT_FLAG) {
                affirm_msg(TABLE_ADDRESS(pt_entry) != 0, "pt_entry:0x%08lx",
                           pt_entry);
                uint32_t phys_address = TABLE_ADDRESS(pt_entry);
                physfree(phys_address);
            }
            // Zero the entry as well
            pt[i] = 0;
        }
    } else {
        /* Currently nothing to free if < USER_MEM_START */
    }
}

/** brief Walks the page directory and frees the entire page directory,
 *        page tables, and all physical frames
 */
void
free_pd_memory( void *pd )
{
    return;
    affirm(is_valid_pd(pd));
    uint32_t **pd_cast = (uint32_t **) pd;

    for (int i = 0; i < PAGE_SIZE / sizeof(uint32_t); ++i) {

        uint32_t *pd_entry = pd_cast[i];

        /* Check page table if entry non-zero */
        if ((uint32_t) pd_entry & PRESENT_FLAG) {
            uint32_t *pt = (uint32_t *) TABLE_ADDRESS(pd_entry);
            free_pt_memory(pt, i);
            sfree(pt, PAGE_SIZE);
        }
    }
}
/** @brief Checks if paget table at index i of a page directory is valid or not.
 *
 *  If the address is < USER_MEM_START, does not validate the address.
 *  TODO perhaps find a way to validate such addresses as well?
 *
 *  @param pt Page table address to check
 *  @param pd_index The index this page table address was stored in the
 *         page directory
 */
static int
is_valid_pt( uint32_t *pt, int pd_index )
{
    return 1;
    /* Basic page table address checks */
    if (!pt) {
        log_warn("pt: %p is NULL!", pt);
        return 0;
    }
    if (!PAGE_ALIGNED(pt)) {
        log_warn("pt: %p is not page aligned!", pt);
        return 0;
    }
    if ((uint32_t) pt >= USER_MEM_START) {
        log_warn("pt: %p is above USER_MEM_START!", pt);
        return 0;
    }

    /* Iterate over page table and check each entry */
    for (int i = 0; i < PAGE_SIZE / sizeof(uint32_t); ++i) {
        uint32_t pt_entry = pt[i];
        //if (pd_index == 0x3ff) {
        //  if (pt_entry) log("pt_entry:0x%08lx", pt_entry);
        //}

        /* Check only if entry is non-NULL, ignoring bottom 12 bits */
        if (TABLE_ADDRESS(pt_entry)) {

            uint32_t phys_address = TABLE_ADDRESS(pt_entry);

            /* Present bit must be set */
            if (!(pt_entry & PRESENT_FLAG)) {
                log_warn("pt at address: %p has non-present frame physical "
                         "address: %p with pt_entry: 0x%08lx at "
                         "index: 0x%08lx",
                         pt, (uint32_t *) phys_address, pt_entry, i);
                log_warn("faulting address:%p", &pt_entry);
                return 0;
            }
            /* pt holds physical frames for user memory */
            if (pd_index >= (USER_MEM_START >> PAGE_DIRECTORY_SHIFT)) {

                /* Frame must be a valid physical address by physalloc */
                if (!is_physframe(phys_address)) {
                    log_warn("pt at address: %p has invalid frame physical "
                             "address: %p with pt_entry: 0x%08lx at "
                             "index: 0x%08lx",
                             pt, (uint32_t *) phys_address, pt_entry, i);
                    return 0;
                }
            /* pt holds physical frame in kernel VM */
            } else {

                /* Frame must be < USER_MEM_START */
                if (phys_address >= USER_MEM_START) {
                    log_warn("pt at address: %p has invalid frame physical "
                             "address: %p >= USER_MEM_START with pt_entry: "
                             "0x%08lx at index: 0x%08lx",
                             pt, (uint32_t *) phys_address, pt_entry, i);
                    return 0;
                }
            }
        }
    }
    return 1;
}

/** @brief Checks if supplied page directory is valid or not
 *
 *  TODO keep a record of all page directory addresses ever given out?
 *
 *  @param pd Page directory to check
 *  @return 1 if pd points to a valid page directory, 0 otherwise
 */
int
is_valid_pd( void *pd )
{
    return 1;
    /* Basic page directory address checks */
    if (!pd) {
        log_warn("pd: %p is NULL!", pd);
        return 0;
    }
    if (!PAGE_ALIGNED(pd)) {
        log_warn("pd: %p is not page aligned!", pd);
        return 0;
    }
    if ((uint32_t) pd >= USER_MEM_START) {
        log_warn("pd: %p is above USER_MEM_START!", pd);
        return 0;
    }
    /* Iterate over page directory and check each entry */
    uint32_t **pd_cast = (uint32_t **) pd;
    for (int i = 0; i < PAGE_SIZE / sizeof(uint32_t); ++i) {
        uint32_t *pd_entry = pd_cast[i];

        /* Check only if entry is non-NULL, ignoring bottom 12 bits */
        if (TABLE_ADDRESS(pd_entry)) {
            uint32_t *pt = (uint32_t *) TABLE_ADDRESS(pd_entry);

            /* Present bit must be set */
            if (!((uint32_t) pd_entry & PRESENT_FLAG)) {
                log_warn("pd at address: %p has non-present pt at address: %p "
                         "with pd_entry: 0x%08lx at index: 0x%08lx",
                         pd, pt, pd_entry, i);
                return 0;
            }
            /* Page table at address must be valid */
            if (!is_valid_pt(pt, i)) {
                log_warn("pd at address: %p has invalid pt at address: %p "
                         "with pd_entry: 0x%08lx at index: 0x%08lx",
                         pd, pt, pd_entry, i);
                return 0;
            }
        }
    }
    return 1;
}
