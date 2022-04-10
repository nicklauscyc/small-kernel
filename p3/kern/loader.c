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
#include <x86/cr.h>     /* {get,set}_{cr0,cr3} */
#include <loader.h>
#include <malloc.h>     /* sfree() */
#include <page.h>       /* PAGE_SIZE */
#include <string.h>     /* strncmp, memcpy */
#include <exec2obj.h>   /* exec2obj_TOC */
#include <elf_410.h>    /* simple_elf_t, elf_load_helper */
#include <stdint.h>     /* UINT32_MAX */
#include <task_manager.h>   /* task_new, task_prepare, task_set, STACK_ALIGNED*/
#include <memory_manager.h> /* {disable,enable}_write_protection */
#include <logger.h>     /* log_warn() */
#include <scheduler.h> /* get_running_tid() */
#include <assert.h> /* assert() */
#include <simics.h>
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

	if (offset > exec2obj_userapp_TOC[i].execlen) {
		log_warn("Loader [getbytes]: Offset (%d) is greater than executable "
				 "size (%d)", offset, exec2obj_userapp_TOC[i].execlen);
		return -1;
	}

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
   // disable_write_protection();

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
    //enable_write_protection();

	assert(is_valid_pd((void *)get_cr3()));
	return i;
}

/** @brief Puts arguments on stack with format required by _main entrypoint.
 *
 *  This entrypoint is defined in 410user/crt0.c and is used by all user
 *  programs.
 *
 *  Requires that argc and argv are validated
 */
//TODO does not set up stack properly for main
//void _main(int argc, char *argv[], void *stack_high, void *stack_low)
//
static uint32_t *
configure_stack( int argc, char **argv )
{
	/* Set highest addressable byte on stack */
	char *stack_high = (char *) UINT32_MAX;
	assert((uint32_t) stack_high == 0xFFFFFFFF);

	/* Set lowest addressable byte on stack */
	char *stack_low = stack_high - PAGE_SIZE + 1;
	assert(PAGE_ALIGNED(stack_low));

	char *esp_char = stack_high - sizeof(uint32_t) + 1;

	/* Transfer argv onto user stack */
	char *user_stack_argv[argc];
	memset(user_stack_argv, 0, argc * sizeof(char *));

	/* Put the char * onto the user stack */
	for (int i = 0; i < argc; ++i) {
		esp_char -= USER_STR_LEN;
		log("string of argv at address:%p", esp_char);
		assert(STACK_ALIGNED(esp_char));
		memset(esp_char, 0, USER_STR_LEN);
		memcpy(esp_char, argv[argc - 1 - i], strlen(argv[argc - 1 - i]));
		user_stack_argv[argc - 1 - i] = esp_char;
	}
	uint32_t *esp = (uint32_t *) esp_char;

    /* Put the null terminated char ** onto the user stack */
	*(--esp) = 0;
	for (int i = 0; i < argc; ++i) {
		*(--esp) = (uint32_t) user_stack_argv[argc - 1 - i];
	}
	/* Save value of char **argv */
	char **argv_arg = (char **) esp;

	/* Put stack_low onto stack */
	*(--esp) = (uint32_t) stack_low;

	/* Put stack_high onto stack */
	*(--esp) = (uint32_t) stack_high;

    /* Put argv_arg onto the user stack */
	*(--esp) = (uint32_t) argv_arg;

	/* Put argc onto stack */
	*(--esp) = argc;

    /* Functions expect esp to point to return address on entry.
     * Therefore we just point it to some garbage, since _main
     * is never supposed to return. */
    esp--;

	log("stack top:%p", esp);
	log("argc:%d", *(esp + 1));
	log("argv[0]:%s", (char *) **((char ***)esp + 2));
	log("stack_hi:%p", (char *) *(esp + 3));
	log("stack_lo:%p", (char *) *(esp + 4));

	return esp;




    ///* TODO: Consider writing this with asm, as it might be simpler. */

    ///* TODO: In the future, when "receiver" function is implemented, loader
    // * should also add entry point, user registers and data segment selectors
    // * on the stack. For registers, just initialize most to 0 or something. */

    ///* As a pointer to uint32_t, it must point to the lowest address of
    // * the value. */
    //uint32_t *esp = (uint32_t *)(UINT32_MAX - (sizeof(uint32_t) - 1));
	//assert((uint32_t) esp % 4 == 0);

    //*esp = argc;

    //if (argc == 0) {
    //    *(--esp) = 0;
	//	assert((uint32_t) esp % 4 == 0);
    //    return esp;
    //}

    //esp -= argc; /* sizeof(char *) == sizeof(uint32_t *) */
	//assert((uint32_t) esp % 4 == 0);
	//// TODO what if argv has a string
    //memcpy(esp, argv, argc * sizeof(char *)); /* Put argv on stack */
    //esp--;
	//assert((uint32_t) esp % 4 == 0);

    //*(esp--) = UINT32_MAX; /* Put stack_high on stack */
	//assert((uint32_t) esp % 4 == 0);

    //*(esp) = UINT32_MAX - PAGE_SIZE - 1; /* Put stack_low on stack */

	//assert((uint32_t) esp % 4 == 0);
	//return esp;
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
		//assert(is_valid_pd(old_pd));
		free_pd_memory(old_pd);
		sfree(old_pd, PAGE_SIZE);
	}


	/* Update page directory, enable VM if necessary */
	if (activate_task_memory(pid) < 0) {
		return -1;
	}
	assert(is_valid_pd(get_tcb_pd(find_tcb(tid))));

    if (transplant_program_memory(&se_hdr) < 0) {
        return -1;
	}
	assert(is_valid_pd(get_tcb_pd(find_tcb(tid))));


    uint32_t *esp = configure_stack(argc, argv);
	assert(is_valid_pd(get_tcb_pd(find_tcb(tid))));



	/* If this is the first task we must activate it */
	if (first_task) {
	assert(is_valid_pd(get_tcb_pd(find_tcb(tid))));


		task_set_active(tid);
	}
	assert(is_valid_pd(get_tcb_pd(find_tcb(tid))));


	first_task = 0;

	/* Start the task */
	task_start(tid, (uint32_t)esp, se_hdr.e_entry);


	panic("execute_user_program does not return");
	return -1;
}
