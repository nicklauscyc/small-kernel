/** Virtual memory manager
 *
 * */

#include <memory_manager.h>
#include <stdint.h>     /* uint32_t */
#include <stddef.h>     /* NULL */
#include <malloc.h>     /* smemalign, sfree */
#include <elf_410.h>    /* simple_elf_t */
#include <assert.h>     /* assert, affirm */
#include <page.h>       /* PAGE_SIZE */
#include <x86/cr.h>     /* {get,set}_cr0 */
#include <string.h>     /* memset, memcpy */
#include <common_kern.h>/* USER_MEM_START */

#include <simics.h>     /* lprintf */

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

/* FIXME: Temporary variable for enabling allocation of physical frames.
 *        Only to be used for user memory. Starts at USER_MEM_START, where
 *        the first phys frames are available. */
static uint32_t next_free_phys_frame;

/** Whether page is read only or also writable. */
typedef enum write_mode write_mode_t;
enum write_mode { READ_ONLY, READ_WRITE };

static int get_next_free_frame( uint32_t *frame );
static uint32_t num_free_frames( void );
static uint32_t *get_pte( uint32_t **pd, uint32_t virtual_address );
static void allocate_frame( uint32_t **pd,
        uint32_t virtual_address, write_mode_t write_mode );
static int allocate_region( void *pd, void *start,
        uint32_t len, write_mode_t write_mode );
static void enable_paging( void );
//static void disable_paging( void );
static int valid_memory_regions( simple_elf_t *elf );

/** Initialize virtual memory. */
int
vm_init( void )
{
    next_free_phys_frame = USER_MEM_START;

    assert((next_free_phys_frame & (PAGE_SIZE - 1)) == 0);

    return 0;
}

/** Allocate memory for new task at given page table directory.
 *  Assumes page table directory is empty. Sets appropriate
 *  read/write permissions. To copy memory over, set the WP flag
 *  in the CR0 register to 0 - this will make it so that write
 *  protection is ignored by the paging mechanism.
 *
 *  Allocated pages are initialized to 0.
 *
 *  TODO: Implement ZFOD here. (Handler should probably be defined
 *  elsewhere, though)
 *  */
int
vm_task_new ( void *pd, simple_elf_t *elf,
        uint32_t stack_lo, uint32_t stack_len )
{
    affirm(pd);
	lprintf("elf->e_entry:%lx", elf->e_entry);


    //lprintf("Direct mapping kernel");

    /* Direct map all 16MB for kernel, setting correct permission bits */
    for (uint32_t addr = 0; addr < USER_MEM_START; addr += PAGE_SIZE) {
        uint32_t *pte = get_pte(pd, addr);
		if (!pte) return -1;
	    if (addr == 0) {
            *pte = addr | PE_UNMAPPED; /* Leave NULL unmapped. */
        } else {
            *pte = addr | PE_KERN_WRITABLE; /* PE_KERN_WRITABLE FIXME FIXME FIXME FIXME FIXME FIXME FIXME */
        }
    }

    /* Allocate regions with appropriate read/write permissions.
     * TODO: Free allocated regions if later allocation fails. */
    int i = 0;

    if (!valid_memory_regions(elf))
        return -1;

    //lprintf("Allocating regions txt");
    i += allocate_region(pd, (void *)elf->e_txtstart, elf->e_txtlen, READ_ONLY);
    //lprintf("Allocating regions data");
    i += allocate_region(pd, (void *)elf->e_datstart, elf->e_datlen, READ_WRITE);
    //lprintf("Allocating regions rodata");
    i += allocate_region(pd, (void *)elf->e_rodatstart, elf->e_rodatlen, READ_ONLY);
    //lprintf("Allocating regions bss");
    i += allocate_region(pd, (void *)elf->e_bssstart, elf->e_bsslen, READ_WRITE);
    //lprintf("Allocating regions stack??");
    i += allocate_region(pd, (void *)stack_lo, stack_len, READ_WRITE);
    //lprintf("Allocated all regions");

    return i;
}

/** @brief Sets new page table directory and enables paging. */
void
vm_enable_task( void *pd )
{
    lprintf("get_cr3");
    uint32_t cr3 = get_cr3();
    /* Unset top 20 bits where new page table will be stored.*/
    cr3 &= PAGE_SIZE - 1;
    cr3 |= (uint32_t)pd;

    //lprintf("set_cr3");
    set_cr3(cr3);
    //lprintf("done");

    //lprintf("disable_paging");
    //disable_paging(); /* FIXME: Remove, annoying compiler*/
    //lprintf("enable_paging");
    enable_paging();
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

/* ----- HELPER FUNCTIONS ----- */

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

    if (num_free_frames() == 0)
        return -1;

    next_free_phys_frame += PAGE_SIZE;

    assert((free_frame & (PAGE_SIZE - 1)) == 0);

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
get_pte( uint32_t **pd, uint32_t virtual_address )
{
    affirm(pd);
    uint32_t pd_index = PD_INDEX(virtual_address);
    uint32_t pt_index = PT_INDEX(virtual_address);

	uint32_t *ptp = pd[pd_index];

    if (!((uint32_t)pd[pd_index] & PRESENT_FLAG)) {
        /* Allocate new page table, which must be page-aligned */
		ptp	= smemalign(PAGE_SIZE, PAGE_SIZE);
		if (!ptp) {
			return ptp;
		}
		assert(((uint32_t)ptp & 0xfff) == 0);
        pd[pd_index] = ptp;

        /* Initialize all page table entries as non-present */
        memset(pd[pd_index], 0, PAGE_SIZE);

        /* Set all page directory entries as writable, determine
         * whether truly writable in page table entry. */
        pd[pd_index] = (uint32_t *)((uint32_t)pd[pd_index] | PE_USER_WRITABLE);
    }

    /* Clear out bottom 12 bits */
    ptp = (uint32_t *)((uint32_t)ptp & ~(PAGE_SIZE - 1));

	/* the entry address is different since we don't use the bits in the
	 * page table entry address
	 */
	return ptp + pt_index;
}

/** Allocate new frame at given virtual memory address.
 *  Allocates page tables on demand.
 *
 *  If memory location already had a frame, checks whether it's allocated with
 *  the same flags as this function would set.
 *  */
static void
allocate_frame( uint32_t **pd, uint32_t virtual_address, write_mode_t write_mode )
{
    affirm(pd);

    uint32_t *pte = get_pte(pd, virtual_address);

    if ((*pte & PRESENT_FLAG) != 0) { /* if allocated */
        /* Ensure it's allocated with same flags. */
        if (write_mode == READ_WRITE)
            affirm((*pte & (PAGE_SIZE - 1)) == PE_USER_WRITABLE);
        else
            affirm((*pte & (PAGE_SIZE - 1)) == PE_USER_READABLE);

        return;
    }
    uint32_t free_frame;
    affirm(get_next_free_frame(&free_frame) == 0);

    //lprintf("Allocated physical frame at %p", (void *) free_frame);
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

    if (write_mode == READ_WRITE)
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
 *  @arg pd    Pointer to page directory
 *  @arg start  Virtual memory addess for start of region to be allocated
 *  @arg len    Length of region to be allocated
 *  @arg write_mode 0 if read-only region, non-zero value if writable
 *
 *  @return 0 on success, negative value on failure.
 *  */
static int
allocate_region( void *pd, void *start, uint32_t len, write_mode_t write_mode )
{
    uint32_t pages_to_alloc = (len + PAGE_SIZE - 1) / PAGE_SIZE;

    /* Ensure we have enough free frames to fulfill request */
    if (num_free_frames() < pages_to_alloc)
        return -1;

    /* FIXME: Do we have any guarantee memory regions are page aligned?
     *        They should be, to some extent. At the very least, 2 memory
     *        regions should not be intersect with the same page, as they
     *        could require distinct permissions. THis might not be the case
     *        for data and bss, though, as both are read-write sections. */

    //lprintf("Allocating region at %p, len %lu", (void *)start, len);


    uint32_t u_start = (uint32_t)start;
    /* Allocate 1 frame at a time. */
    for (int i = 0; i < pages_to_alloc; ++i) {
        //lprintf("Allocating frame at %p", (void *)(u_start + PAGE_SIZE * i));
        allocate_frame((uint32_t **)pd, u_start + PAGE_SIZE * i, write_mode);
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
	lprintf("enabled paging");
}

/** @brief Disables paging mechanism. */
//static void
//disable_paging( void )
//{
//	uint32_t current_cr0 = get_cr0();
//	set_cr0(current_cr0 & (~PAGING_FLAG));
//}
