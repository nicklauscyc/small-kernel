/**
 * The 15-410 kernel project.
 * @name loader.c
 *
 * Functions for the loading
 * of user programs from binary
 * files should be written in
 * this file. The function
 * elf_load_helper() is provided
 * for your use.
 *
 * The loader should never interact directly with
 * virtual memory. Rather it should call functions
 * defined in the process mananger module (which itself
 * will be responsible for talking to the VM module).
 *
 */
/*@{*/

/* --- Includes --- */
#include <string.h>
#include <stdio.h>
#include <malloc.h>
#include <exec2obj.h>
#include <loader.h>
#include <elf_410.h>


/* --- Local function prototypes --- */


/** Copies data from a file into a buffer.
 *
 *  @param filename   the name of the file to copy data from
 *  @param offset     the location in the file to begin copying from
 *  @param size       the number of bytes to be copied
 *  @param buf        the buffer to copy the data into
 *
 *  Buf must be page aligned and have size multiple of PAGE_SIZE.
 *  Getbytes zeroes out unused bytes in a given page.
 *
 * @return number of bytes copied on success. Negative value on failure.
 */
static int
getbytes( const char *filename, int offset, int size, char *buf )
{
    if (!filename || !buf || offset < 0 || size < 0)
        return -1;

    /* Find file in TOC */
    int i;
    exec2obj_userapp_TOC_entry entry;
    for (i=0; i < MAX_NUM_APP_ENTRIES; ++i) {
        entry = exect2obj_userapp_TOC[i];
        if (strncmp(filename, entry.execname, MAX_EXECNAME_LEN) == 0) {
            break;
        }
    }

    if (i == MAX_NUM_APP_ENTRIES)
        return -1; /* Executable not found */

    if (offset + size >= entry.execlen)
        return -1; /* Asking for more bytes than are available */

    memcpy(buf, entry.execbytes + offset, size);

    /* Since we require that memory regions be page aligned,
     * we know buf % PAGE_SIZE == 0. We can use this to zero
     * out leftover bytes at region boundaries. */
    int leftover_bytes = PAGE_SIZE - (size % PAGE_SIZE);
    memset(buf + size, 0, leftover_bytes);

    return 0;
}

/** @brief Transplants program data into virtual memory.
 *  Assumes paging is enabled and that virtual memory
 *  has been setup.
 *
 *  @arg se_hdr Elf header
 *
 *  @return 0 on sucess, negative value on failure.
 *  */
static int
transplant_program_memory( simple_elf_t *se_hdr )
{
    /* Disable write-protection temporarily so we may
     * copy data into read-only regions. */
    disable_write_protection();

    // FIXME: This error checking is kinda hacky
    int i = 0;
    /* We rely on the fact that virtual-memory is
     * enabled to "transplant" program data. Notice
     * that this is only possible because program data is
     * resident on kernel memory which is direct-mapped. */
	i += getbytes(se_hdr->e_fname,
            (unsigned int) se_hdr->e_txtoff,
            (unsigned int) se_hdr->e_txtlen,
	        (char *) se_hdr->e_txtstart);

	i += getbytes(se_hdr->e_fname,
            (unsigned int) se_hdr->e_datoff,
            (unsigned int) se_hdr->e_datlen,
	        (char *) se_hdr->e_datstart);

	i += getbytes(se_hdr->e_fname,
	        (unsigned int) se_hdr->e_rodatoff,
            (unsigned int) se_hdr->e_rodatlen,
	        (char *) se_hdr->e_rodatstart);

    /* Re-enable write-protection bit. */
    enable_write_protection();

	return i;
}

/** @brief Run a user program indicated by filename.
 *         Assumes virtual memory module has been initialized.
 *
 *  @arg fname Name of program to run.
 *
 *  @return 0 on success, negative value on error.
 */
int
execute_user_program( const char *fname )
{
	/* Load user program information */
	simple_elf_t se_hdr;
	if (elf_load_helper(&se_hdr, fname) == ELF_NOTELF)
        return -1;

    // TODO: Create task, set it active and transplant memory.
    // Finally, call run_task, a kind of process_switch function
    // new_task(se_hdr, 0, 0)
    // set_active_task(0);
    // transplant_memory;
    // Write arguments into stack.
    // process_set(0, stack_lo, se_hdr->entrypoint);

	return 0;
}

/*@}*/
