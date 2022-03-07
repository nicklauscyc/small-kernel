/**
 * Virtual memory manager
 *
 *
 * */
#include <simics.h> /* lprintf() */
#include <stdint.h>
#include <stddef.h>
#include <malloc.h>
#include <elf/elf_410.h>
#include <x86/cr.h> /* set_cr0() */
#include <common_kern.h> /* machine_phys_frame() */
#include <string.h> /* memset() */

#define PAGE_ENTRY_SIZE 64
#define PAGE_TABLE_SIZE (1024 * PAGE_ENTRY_SIZE)

#define PAGE_DIRECTORY_INDEX 0xFFC00000
#define PAGE_TABLE_INDEX 0x003FF000
#define PAGE_OFFSET 0x00000FFF

int num_page_frames = 0;

#define NUM_ENTRIES 1024
/* This is 4KByte = 4 * 1024 Byte total */
void *PAGE_TABLE[NUM_ENTRIES];

/* Page directory and page table coincidentally have
 * the same size. */
#define PAGE_DIRECTORY_SIZE PAGE_TABLE_SIZE

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
static void *next_free_phys_frame;

#define PAGING_OFF 0x7FFFFFFF
#define PAGING_ON 0x80000000
void
paging_on( void )
{
	uint32_t current_cr0 = get_cr0();
	set_cr0(current_cr0 | PAGING_ON);
}

void
paging_off( void )
{
	uint32_t current_cr0 = get_cr0();
	set_cr0(current_cr0 & PAGING_OFF);
}

/** Initialize virtual memory */
int
vm_init( void )
{
    next_free_phys_frame = (void *) (uint32_t) USER_MEM_START;
	num_page_frames = machine_phys_frames();
	memset(PAGE_TABLE, 0, sizeof(void *) * NUM_ENTRIES);
	lprintf("PAGE_TABLE address:%p", &PAGE_TABLE);

	/* Need to have a page directory here */
    return 0;
}

/** Allocate new pages in a given process' virtual memory. */
/* Currently only 1 task supported, so no need to index based on tid */
int
vm_new_pages ( void *ptd_start, void *base, int len )
{
    return -1;
}

/** Allocate memory for new task at given page table directory.
 *
 *  TODO: Create a subroutine suitable for cloning. */
int
vm_new_task ( void *ptd_start, simple_elf_t *elf )
{
    /* Allocate phys frames and assign to virtual memory */

    /* Allocate rodata, text with read-only permissions */

    /* Allocate code with execute permissions */

    /* Allocate data with read-write permissions */

    return -1;
}

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
