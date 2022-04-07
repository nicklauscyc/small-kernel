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
#include <malloc.h>     /* sfree() */
#include <page.h>       /* PAGE_SIZE */
#include <string.h>     /* strncmp, memcpy */
#include <exec2obj.h>   /* exec2obj_TOC */
#include <elf_410.h>    /* simple_elf_t, elf_load_helper */
#include <stdint.h>     /* UINT32_MAX */
#include <task_manager.h>   /* task_new, task_prepare, task_set */
#include <memory_manager.h> /* {disable,enable}_write_protection */
#include <logger.h>     /* log_warn() */
#include <scheduler.h> /* get_running_tid() */
#include <assert.h> /* assert() */

/* --- Local function prototypes --- */

/* first_task is 1 if there is currently no user task running, so
 * execute_user_program will do some initialization
 */
static int first_task = 1;

/* TODO: Move this to a helper file.
 *
 * Having a helper means we evaluate the arguments before expanding _MIN
 *  and therefore avoid evaluating A and B multiple times. */
#define _MIN(A, B) ((A) < (B) ? (A) : (B))
#define MIN(A,B) _MIN(A,B)

/** Copies data from a file into a buffer.
 *
 *  @param filename   the name of the file to copy data from
 *  @param offset     the location in the file to begin copying from
 *  @param size       the number of bytes to be copied
 *  @param buf        the buffer to copy the data into
 *
 * @return number of bytes copied on success. Negative value on failure.
 */
int
getbytes( const char *filename, int offset, int size, char *buf )
{
    if (size == 0)
        return 0; /* Nothing to copy*/

    if (!filename || !buf || offset < 0 || size < 0) {
        log_warn("Loader [getbytes]: Invalid arguments.");
        return -1;
    }

    /* Find file in TOC */
    int i;
    for (i=0; i < exec2obj_userapp_count; ++i) {
        if (strncmp(filename, exec2obj_userapp_TOC[i].execname, MAX_EXECNAME_LEN) == 0) {
            break;
        }
    }

    if (i == exec2obj_userapp_count) {
        log_warn("Loader [getbytes]: Executable not found");
        return -1;
    }

    ///* FIXME: Spec is unclear. Should we copy as much as we can, or should
    // * only copy if there are enough bytes in the executable? */
    //if (offset + size >= exec2obj_userapp_TOC[i].execlen)
    //    return -1; /* Asking for more bytes than are available */

    int bytes_to_copy = MIN(size, exec2obj_userapp_TOC[i].execlen - offset);

    memcpy(buf, exec2obj_userapp_TOC[i].execbytes + offset, bytes_to_copy);

    return bytes_to_copy;
}

/** @brief Transplants program data into virtual memory.
 *  Assumes paging is enabled and that virtual memory
 *  has been setup.
 *
 *  @param se_hdr Elf header
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

    /* TODO: Zero out bytes between memory regions */

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
//TODO does not set up stack properly for main
//void _main(int argc, char *argv[], void *stack_high, void *stack_low)
//
static uint32_t *
configure_stack( int argc, char **argv )
{
    /* TODO: Consider writing this with asm, as it might be simpler. */

    /* TODO: In the future, when "receiver" function is implemented, loader
     * should also add entry point, user registers and data segment selectors
     * on the stack. For registers, just initialize most to 0 or something. */

    /* As a pointer to uint32_t, it must point to the lowest address of
     * the value. */
    uint32_t *esp = (uint32_t *)(UINT32_MAX - (sizeof(uint32_t) - 1));
	assert((uint32_t) esp % 4 == 0);

    *esp = argc;

    if (argc == 0) {
        *(--esp) = 0;
		assert((uint32_t) esp % 4 == 0);
        return esp;
    }

    esp -= argc; /* sizeof(char *) == sizeof(uint32_t *) */
	assert((uint32_t) esp % 4 == 0);
	// TODO what if argv has a string
    memcpy(esp, argv, argc * sizeof(char *)); /* Put argv on stack */
    esp--;
	assert((uint32_t) esp % 4 == 0);

    *(esp--) = UINT32_MAX; /* Put stack_high on stack */
	assert((uint32_t) esp % 4 == 0);

    *(esp) = UINT32_MAX - PAGE_SIZE - 1; /* Put stack_low on stack */

    /* Functions expect esp to point to return address on entry.
     * Therefore we just point it to some garbage, since _main
     * is never supposed to return. */
    esp--;
	assert((uint32_t) esp % 4 == 0);
	return esp;
}

/** @brief Run a user program indicated by fname.
 *
 *  This function requires no synchronization as it is only
 *  meant to be used to load the starter program (when we have
 *  a single thread) and for the syscall exec(). We disable calls to
 *  exec() when there is more than 1 thread in the invoking task.
 *
 *  TODO don't think this is the best requires
 *  @req that fname and argv are in kernel memory, unaffected by
 *       parent directory
 *
 *  @param fname Name of program to run.
 *  @return 0 on success, negative value on error.
 */
int
execute_user_program( const char *fname, int argc, char **argv)
{
	log_info("Executing: %s", fname);

	/* Load user program information */
	simple_elf_t se_hdr;
    if (elf_check_header(fname) == ELF_NOTELF) {
        return -1;
	}
    if (elf_load_helper(&se_hdr, fname) == ELF_NOTELF) {
        return -1;
	}
    uint32_t pid, tid;

	/* First task, so create a new task */
	if (first_task) {
		if (create_task(&pid, &tid, &se_hdr) < 0)
			return -1;

	/* Not the first task, so we replace the current running task */
	} else {
		pid = get_pid();
		tid = get_running_tid();

		/* Create new pd */
		uint32_t stack_lo = UINT32_MAX - PAGE_SIZE + 1;
		uint32_t stack_len = PAGE_SIZE;
		void *new_pd = new_pd_from_elf(&se_hdr, stack_lo, stack_len);
		if (!new_pd) {
			return -1;
		}
		void *old_pd = swap_task_pd(new_pd);
		free_pd_memory(old_pd);
		sfree(old_pd, PAGE_SIZE);
	}
	/* Update page directory, enable VM if necessary */
	if (activate_task_memory(pid) < 0) {
		return -1;
	}
    if (transplant_program_memory(&se_hdr) < 0) {
        return -1;
	}

    uint32_t *esp = configure_stack(argc, argv);

	/* If this is the first task we must activate it */
	if (first_task) {
		task_set_active(tid);
	}
	first_task = 0;

	/* Start the task */
	task_start(tid, (uint32_t)esp, se_hdr.e_entry);

	panic("execute_user_program does not return");
	return -1;
}
