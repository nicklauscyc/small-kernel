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
#include <loader.h>
#include <page.h>       /* PAGE_SIZE */
#include <string.h>     /* strncmp, memcpy */
#include <exec2obj.h>   /* exec2obj_TOC */
#include <elf_410.h>    /* simple_elf_t, elf_load_helper */
#include <limits.h>     /* UINT_MAX */
#include <task_manager.h>   /* task_new, task_prepare, task_set */
#include <memory_manager.h> /* {disable,enable}_write_protection */


/* --- Local function prototypes --- */

/** Copies data from a file into a buffer.
 *
 *  @param filename   the name of the file to copy data from (null-terminated)
 *  @param offset     the location in the file to begin copying from
 *  @param size       the number of bytes to be copied
 *  @param buf        the buffer to copy the data into
 *
 *  Buf must be page aligned and have size multiple of PAGE_SIZE.
 *  Getbytes zeroes out unused bytes in a given page.
 *
 * @return number of bytes copied on success. Negative value on failure.
 */
int
getbytes( const char *filename, int offset, int size, char *buf )
{
    if (!filename || !buf || offset < 0 || size < 0)
        return -1;

    /* Find file in TOC */
    int i;
    for (i=0; i < MAX_NUM_APP_ENTRIES; ++i) {
        if (strncmp(filename, exec2obj_userapp_TOC[i].execname, MAX_EXECNAME_LEN) == 0) {
            break;
        }
    }

    if (i == MAX_NUM_APP_ENTRIES)
        return -1; /* Executable not found */

    if (offset + size >= exec2obj_userapp_TOC[i].execlen)
        return -1; /* Asking for more bytes than are available */

    memcpy(buf, exec2obj_userapp_TOC[i].execbytes + offset, size);

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

/** @brief Puts arguments on stack with format required by _main entrypoint.
 *
 *  This entrypoint is defined in 410user/crt0.c and is used by all user
 *  programs.
 *  */
static uint32_t *
configure_stack( int argc, char **argv )
{

    /* TODO: In the future, when "receiver" function is implemented, loader
     * should also add entry point, user registers and data segment selectors
     * on the stack. For registers, just initialize most to 0 or something. */

    uint32_t *esp = (uint32_t *)UINT_MAX;

    *(esp) = argc;
    esp -= argc; /* sizeof(char *) == sizeof(uint32_t *) */
    memcpy(esp, argv, argc * sizeof(char *)); /* Put argv on stack */
    esp--;

    *(esp--) = UINT_MAX; /* Put stack_high on stack */
    *(esp) = UINT_MAX - PAGE_SIZE; /* Put stack_low on stack */

    /* Functions expect esp to point to return address on entry.
     * Therefore we just point it to some garbage, since _main
     * is never supposed to return. */
    esp--;

    return esp;
}

/** @brief Run a user program indicated by filename.
 *         Assumes virtual memory module has been initialized.
 *
 *  @arg fname Name of program to run.
 *
 *  @return 0 on success, negative value on error.
 */
int
execute_user_program( const char *fname, int argc, char **argv )
{
	/* Load user program information */
	simple_elf_t se_hdr;
    if (elf_check_header(fname) == ELF_NOTELF)
        return -1;

    if (elf_load_helper(&se_hdr, fname) == ELF_NOTELF)
        return -1;

    /* FIXME: Hard coded pid and tid for now */
    if (task_new(0, 0, &se_hdr) < 0)
        return -1;

    /* Enable VM */
    if (task_prepare(0) < 0)
        return -1;

    if (transplant_program_memory(&se_hdr) < 0)
        return -1;

    uint32_t *esp = configure_stack(argc, argv);

    task_set(0, (uint32_t)esp, se_hdr.e_entry);

	return 0;
}


/* we try to use the physical addresses */

/*@}*/
