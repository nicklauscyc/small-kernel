/**
 * Virtual memory manager
 *
 *
 * */

#include <stdint.h>
#include <stddef.h>
#include <malloc.h>
#include <elf/elf_410.h>

#define PAGE_ENTRY_SIZE 64
#define PAGE_TABLE_SIZE (1024 * PAGE_ENTRY_SIZE)

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

/** Initialize virtual memory */
int
vm_init( void )
{

    next_free_phys_frame = USER_MEM_START;

    return 0;
}

/** Allocate new pages in a given process' virtual memory. */
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
