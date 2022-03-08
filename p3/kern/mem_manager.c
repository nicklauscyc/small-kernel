/** Virtual memory manager
 *
 * */

#include <stdint.h>
#include <stddef.h>
#include <malloc.h> /* smemalign, sfree */
#include <elf/elf_410.h>
#include <assert.h>
#include <page.h>   /* PAGE_SIZE */
#include <x86/cr.h> /* {get,set}_cr0 */
#include <string.h> /* memset */

#define PAGING_FLAG 0x80000000

#define PAGE_DIRECTORY_INDEX 0xFFC00000
#define PAGE_TABLE_INDEX 0x003FF000
#define PAGE_OFFSET 0x00000FFF

#define PAGE_DIRECTORY_SHIFT 22
#define PAGE_TABLE_SHIFT 12

/* Get page directory or page table index from a linear address. */
#define PD_INDEX(addr) \
    ((PAGE_DIRECTORY_INDEX & (addr)) >> PAGE_DIRECTORY_SHIFT)
#define PT_INDEX(addr) \
    ((PAGE_TABLE_INDEX & (addr)) >> PAGE_TABLE_SHIFT)

// TODO: Move to appropriate place
/* Flags for page directory and page table entries */
#define PRESENT_FLAG 1 << 0
#define RW_FLAG      1 << 1
#define US_FLAG      1 << 2
#define GLOBAL_FLAG  1 << 8

#define PE_USER_READABLE (PRESENT_FLAG | US_FLAG )
#define PE_USER_WRITABLE (PE_USER_READABLE | RW_FLAG)

#define PE_KERN_READABLE (PRESENT_FLAG | GLOBAL_FLAG | RW_FLAG)
#define PE_KERN_WRITABLE (PE_KERN_READABLE | RW_FLAG)

#define PE_UNMAPPED 0

/** lmm and malloc are responsible for managing kernel virtual
 *  memory. However, that virtual memory must still be associated
 *  to some physical memory (which we must direct map)
 *
 *  For user memory, however, we must manage physical pages starting
 *  at USER_MEM_START up to machine_phys_frames() * PAGE_SIZE.
 *  TODO: Currently no free, but in the future some data structure should manage
 *  this.
 *
 *  To ensure page tables/directories are aligned,
 *  always use smemalign() and sfree() * */

/* FIXME: Temporary variable for enabling allocation of physical frames.
 *        Only to be used for user memory. Starts at USER_MEM_START, where
 *        the first phys frames are available. */
static uint32_t next_free_phys_frame;

/** Initialize virtual memory. */
int
vm_init( void )
{

    next_free_phys_frame = USER_MEM_START;

    assert(next_free_phys_frame & (PAGE_SIZE - 1) == 0);

    return 0;
}

/** Gets new page-aligned physical frame.
 *
 *  @arg frame Memory location in which to store new frame address
 *
 *  @return 0 on success, negative value on failure
 * */
static int
get_next_free_frame( uint32_t *frame )
{
    uint32_t free_frame = next_free_phys_frame;

    if (free_frame >= machine_phys_frames() * PAGE_SIZE)
        return -1;

    next_free_phys_frame += PAGE_SIZE;

    assert(free_frame & (PAGE_SIZE - 1) == 0);

    *frame = free_frame;
    return 0;
}

/** Gets number of remaining free physical frames. */
static uint32_t
num_free_frames( void )
{
    int remaining = machine_phys_frames() - (next_free_phys_frame / PAGE_SIZE);
    affirm(remaining >= 0); /* We should never allocate memory we don't have! */
    return (uint32_t)remaining;
}

/** Gets pointer to page table entry in a given page directory.
 *  Allocates page table if necessary. */
static uint32_t *
get_pte( uint32_t **ptd, uint32_t virtual_address )
{
    affirm(ptd);
    uint32_t pd_index = PD_INDEX(virtual_address);
    uint32_t pt_index = PT_INDEX(virtual_address);

    if (!(ptd[pd_index] & PRESENT_FLAG)) {
        /* Allocate new page table, which must be page-aligned */
        ptd[pd_index] = smemalign(PAGE_SIZE, PAGE_SIZE);
        affirm(ptd[pd_index]);

        /* Initialize all page table entries as non-present */
        memset(ptd[pd_index], 0, PAGE_SIZE);

        /* Set all page directory entries as writable, determine
         * whether truly writable in page table entry. */
        ptd[pd_index] |= PE_USER_WRITABLE;
    }

    uint32_t *page_table = ptd[pd_index];

    return page_table + pt_index;
}

/** Allocate new frame at given virtual memory address.
 *  Allocates page tables on demand.
 *
 *  If memory location already had a frame, this crashes.
 *  */
static void
allocate_frame( uint32_t **ptd, uint32_t virtual_address, int write_mode )
{
    affirm(ptd);

    uint32_t free_frame;
    affirm(get_next_free_frame(&free_frame) == 0);

    uint32_t *pte = get_pte(ptd, virtual_address);
    affirm((page_table[pt_index] & PRESENT_FLAG) == 0); /* Ensure unnalocated */

    *pte = free_frame;

    if (write_mode)
        *pte |= PE_USER_WRITABLE;
    else
        *pte |= PE_USER_READABLE;
}

/** Allocates a memory region in virtual memory.
 *
 *  If there aren't enough physical frames to satisfy allocation
 *  request, region is not allocated and function returns a negative
 *  value.
 *
 *  @arg ptd    Pointer to page directory
 *  @arg start  Virtual memory addess for start of region to be allocated
 *  @arg len    Length of region to be allocated
 *  @arg write_mode 0 if read-only region, non-zero value if writable
 *
 *  @return 0 on success, negative value on failure.
 *  */
static int
allocate_region( void *ptd, void *start, uint32_t len, int write_mode )
{
    /* Ensure we have enough free frames to fulfill request */
    if (num_free_frames() < (len + PAGE_SIZE - 1) / PAGE_SIZE)
        return -1;

    uint32_t curr = (uint32_t)start;

    /* Allocate 1 frame at a time. */
    while (curr < (uint32_t)start + len) {
        allocate_frame((uint32_t **)ptd, uint32_t curr);
        curr += PAGE_SIZE;
    }

    /* Zero out leftover bytes still in page.
     * Because we haven't yet updated the page table base register,
     * we cannot reference the virtual address. As such, we disable
     * paging momentarily and update this page. This has to be done
     * ATOMICALLY.*/

    uint32_t end = (uint32_t)start + len;
    uint32_t *last_pte = get_pte(ptd, end);

    /* ATOMICALLY START */
    vm_disable_paging();

    uint32_t frame = *last_pte & ~(PAGE_SIZE - 1);
    frame += len % PAGE_SIZE;

    /* Finally, set remaining bytes to 0 */
    memset((char *)frame, 0, PAGE_SIZE - (len % PAGE_SIZE));

    vm_enable_paging();
    /* ATOMICALLY END */
}

/** Allocate memory for new task at given page table directory.
 *  Assumes page table directory is empty. Sets appropriate
 *  read/write permissions. To copy memory over, set the WP flag
 *  in the CR0 register to 0 - this will make it so that write
 *  protection is ignored by the paging mechanism.
 *
 *  When initializing memory regions specified in elf header,
 *  zeroes out bytes after after their end but before the page
 *  boundary.
 *
 *  TODO: Implement ZFOD here. (Handler should probably be defined
 *  elsewhere, though)
 *
 *  TODO: Consider checking and returning on error
 *  TODO: Create a subroutine suitable for cloning. */
int
vm_new_task ( void *ptd, simple_elf_t *elf )
{
    // TODO: Implement
    /* Direct map all 16MB for kernel. Reminder to map 0th page as not present.
     * Page fault handler should also be defined so we can panic on null
     * dereference or smth.
     * Set U/S permission bits correctly. */

    /* Allocate regions with appropriate read or read/write permissions
     * and any other sensible defaults. */

    return -1;
}


/* For memory copy purposes we could provide a function which sets the
 * CR0 WP flag. */
void
enable_paging( void )
{
	uint32_t current_cr0 = get_cr0();
	set_cr0(current_cr0 | PAGING_FLAG);
}


void
disable_paging( void )
{
	uint32_t current_cr0 = get_cr0();
	set_cr0(current_cr0 & (~PAGING_FLAG));
}

/** Allocate new pages in a given process' virtual memory. */
int
vm_new_pages ( void *ptd, void *base, int len )
{
    return -1;
}


// FIXME: Having this might allow for poor code organization
/** Translate logical address into physical address.
 *
 *  Note: This can be used by loader to "transplant"
 *  data from elf binary into a new task's virtual
 *  memory. */
void *
translate( void *logical )
{
    // TODO: logical -> linear -> physical
    return NULL;
}
