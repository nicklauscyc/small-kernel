/**
 * Virtual memory manager
 *
 *
 * */

#include <stdint.h>
#include <stddef.h>
#include <malloc.h> /* smemalign, sfree */
#include <elf/elf_410.h>
#include <assert.h>
#include <page.h>   /* PAGE_SIZE */
#include <x86/cr.h> /* {get,set}_cr0 */

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

/** lmm and malloc are responsible for managing kernel virtual
 *  memory. However, that virtual memory must still be associated
 *  to some physical memory (which we must direct map)
 *
 *  For user memory, however, we must manage physical pages starting
 *  at USER_MEM_START up to USER_MEM_START + machine_phys_frames() * PAGE_SIZE.
 *  TODO: Currently no free, but in the future some data structure should manage
 *  this.
 *
 * To ensure page tables/directories are aligned, always use
 *  smemalign() and sfree() * */

/* FIXME: Temporary variable for enabling allocation of physical frames.
 *        Only to be used for user memory. Starts at USER_MEM_START, where
 *        the first phys frames are available. */
static uint32_t next_free_phys_frame;


/** Initialize virtual memory */
int
vm_init( void )
{

    next_free_phys_frame = USER_MEM_START;

    assert(next_free_phys_frame & (PAGE_SIZE - 1) == 0);

    return 0;
}

/* Gets new page-aligned physical frame. */
static uint32_t
get_next_free_frame( void )
{
    uint32_t free_frame = next_free_phys_frame;
    next_free_phys_frame += PAGE_SIZE;

    assert(free_frame & (PAGE_SIZE - 1) == 0);

    return free_frame;
}

/** Allocate new frame at given virtual memory address.
 *  Allocates page tables on demand.
 *
 *  If memory location already had a frame, this crashes.
 *  */
static void
allocate_frame( uint32_t **ptd, uint32_t pd_index, uint32_t pt_index )
{
    affirm(ptd);

    if (!ptd[pd_index]) {
        /* Allocate new page table, which must be page-aligned */
        ptd[pd_index] = smemalign(PAGE_SIZE, PAGE_SIZE);
        affirm(ptd[pd_index]);

        /* TODO: Set flags for pd entry */

    }

    uint32_t *page_table = ptd[pd_index];

    /* FIXME: Only check present bit for PTE
     * Ensure page entry is empty */
    affirm(page_table[pt_index] == 0);

    /* Allocate physical frame and point VM to it.
     * This function sets none of the protection bits. */
    uint32_t free_frame = get_next_free_frame();

    // TODO: STOPPED HERE - Write to page table entry



    // TODO: Write flags (also protection bits should be set to READ|WRITE)
    //       Create a function to abstract flag setting (or multiple)


}

static void
allocate_region( void *ptd, void *start, uint32_t len )
{
    // TODO: Affirm we have enough phys frames

    uint32_t curr = (uint32_t)start;
    uint32_t end = curr + len;

    /* Allocate 1 frame at a time. */
    while (curr < (uint32_t)start + len) {
        allocate_frame((uint32_t **)ptd, PD_INDEX(curr), PT_INDEX(curr));
        curr += PAGE_SIZE;
    }

    /* Zero out leftover bytes still in page */
    /* FIXME: I don't think this does what it's supposed to */
    memset((char *)start + len, 0, curr - end);
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
    /* NOTE: We don't actually have to disable paging here. */

    // TODO: Implement
    /* Direct map all 16MB for kernel. Reminder to map 0th page as not present.
     * Page fault handler should also be defined so we can panic on null
     * dereference or smth.
     * Set U/S permission bits correctly. */

    /* Allocate regions with appropriate read or read/write permissions
     * and any other sensible defaults. */

    return -1;
}

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
