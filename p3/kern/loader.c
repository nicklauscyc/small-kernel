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
#include <x86/asm.h> /* enable_interrupts(), disable_interrupts() */
#include <iret_travel.h> /* iret_travel() */
#include <task_manager_internal.h>
#include <eflags.h>	/* get_eflags*/
#include <seg.h>	/* SEGSEL_... */
#include <common_kern.h> /* USER_MEM_START */
#include <lib_memory_management/memory_management.h> /* _new_pages */
/* --- Local function prototypes --- */


/* TODO: Move this to a helper file.
 *
 * Having a helper means we evaluate the arguments before expanding _MIN
 *  and therefore avoid evaluating A and B multiple times. */
#define _MIN(A, B) ((A) < (B) ? (A) : (B))
#define MIN(A,B) _MIN(A,B)

int configure_initial_task_stack( tcb_t *tcbp, uint32_t user_esp,
	uint32_t entry_point, void *user_pd );

int register_with_simics( uint32_t tid, char *fname );

int load_user_program_info(simple_elf_t *se_hdrp, char *fname);

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

static void
zero_out_memory_region( uint32_t start, uint32_t len )
{
	uint32_t align_end = ((start + len + PAGE_SIZE - 1) / PAGE_SIZE) * PAGE_SIZE;
	memset((void *)start, 0, align_end - start);
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
	/* TODO: NICK Swap cr0 on context switch, for now just disabling
	 * interrupts. */
	disable_interrupts();

    /* Disable write-protection temporarily so we may
     * copy data into read-only regions. */
	disable_write_protection();

    /* Zero out bytes between memory regions (as well as inside them)  */
    int i = 0;
	zero_out_memory_region(se_hdr->e_txtstart, se_hdr->e_txtlen);
	zero_out_memory_region(se_hdr->e_rodatstart, se_hdr->e_rodatlen);
	zero_out_memory_region(se_hdr->e_datstart, se_hdr->e_datlen);
	zero_out_memory_region(se_hdr->e_bssstart, se_hdr->e_bsslen);

    /* We rely on the fact that virtual-memory is
     * enabled to "transplant" program data. Notice
     * that this is only possible because program data is
     * resident on kernel memory which is direct-mapped. */
	i = getbytes(se_hdr->e_fname,
            (unsigned int) se_hdr->e_txtoff,
            (unsigned int) se_hdr->e_txtlen,
	        (char *) se_hdr->e_txtstart);
	if (i < 0)
		return -1;

	i = getbytes(se_hdr->e_fname,
	        (unsigned int) se_hdr->e_rodatoff,
            (unsigned int) se_hdr->e_rodatlen,
	        (char *) se_hdr->e_rodatstart);
	if (i < 0)
		return -1;

	i = getbytes(se_hdr->e_fname,
            (unsigned int) se_hdr->e_datoff,
            (unsigned int) se_hdr->e_datlen,
	        (char *) se_hdr->e_datstart);
	if (i < 0)
		return -1;


    /* Re-enable write-protection bit. */
    enable_write_protection();


	if (is_multi_threads()) {
		enable_interrupts();
	}

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
static uint32_t *
configure_stack( int argc, char **argv )
{
	/* Set highest addressable byte on stack */
	char *stack_high = (char *) UINT32_MAX;
	assert((uint32_t) stack_high == 0xFFFFFFFF);

	/* Set lowest addressable byte on stack */
	char *stack_low = stack_high - USER_THREAD_STACK_SIZE + 1;
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

		affirm(esp_char);
		affirm(argv);
		affirm(argv[argc - 1 - i]);

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

	/* Store arguments on stack */
	*(--esp) = (uint32_t) stack_low;
	*(--esp) = (uint32_t) stack_high;
	*(--esp) = (uint32_t) argv_arg;
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
execute_user_program( char *fname, int argc, char **argv)
{
	log_warn("executing task fname:'%s'", fname);

	/* Validate execname */
	if (!is_valid_null_terminated_user_string(fname, USER_STR_LEN)) {
		return -1;
	}
	/* Validate argvec */
	int counted_argc = 0;
	if (!(counted_argc = is_valid_user_argvec(fname, argv))) {
		return -1;
	}
	if (counted_argc != argc) {
		return -1;
	}

    /* Transfer execname to kernel stack so unaffected by page directory */
	char kern_stack_execname[USER_STR_LEN];
	memset(kern_stack_execname, 0, USER_STR_LEN);
	memcpy(kern_stack_execname, fname, strlen(fname));

	/* char array to store each argvec string on kernel stack */
	char *kern_stack_args = smalloc(NUM_USER_ARGS * USER_STR_LEN);
    if (!kern_stack_args) {
        return -1;
    }
	memset(kern_stack_args, 0, NUM_USER_ARGS * USER_STR_LEN);

	/* char * array for argvec on kernel stack */
	char *kern_stack_argvec[NUM_USER_ARGS];
	memset(kern_stack_argvec, 0, NUM_USER_ARGS);

	/* Transfer argvec to kernel memory so unaffected by page directory */
	int offset = 0;
	for (int i = 0; argv[i]; ++i) {
		char *arg = argv[i];
		memcpy(kern_stack_args + offset, arg, strlen(arg));
		kern_stack_argvec[i] = kern_stack_args + offset;
		offset += USER_STR_LEN;
	}
	/* Load user program information */
	simple_elf_t se_hdr;
	if (load_user_program_info(&se_hdr, kern_stack_execname) < 0) {
		goto cleanup;
	}
    uint32_t pid, tid;

	/* Not the first task, so we replace the current running task */
	pid = get_pid();
	tid = get_running_tid();

	/* Create new pd */

	void *new_pd = new_pd_from_elf(&se_hdr);
	if (!new_pd) {
		goto cleanup;
	}
	void *old_pd = swap_task_pd(new_pd);

	set_task_name(find_pcb(pid), kern_stack_execname);

	log_warn("process tid:%d, execname:%s", tid, find_pcb(pid)->execname);

	register_with_simics(tid, kern_stack_execname);

	/* If this is the init task, let the world know */
	register_if_init_task(kern_stack_execname, pid);

	/* Update page directory, enable VM if necessary */
	if (activate_task_memory(pid) < 0) {
		goto cleanup_w_pd;
	}

	/* Allocate user stack space */
	uint32_t stack_lo = UINT32_MAX - USER_THREAD_STACK_SIZE + 1;
	if (_new_pages((uint32_t *) stack_lo, USER_THREAD_STACK_SIZE) < 0) {
		goto cleanup_w_pd;
	}

	/* Cleaning up the new page directory also implicitly cleans up the pages
	 * allocated by _new_pages() above */
    if (transplant_program_memory(&se_hdr) < 0) {
		goto cleanup_w_pd;
	}
    uint32_t *esp = configure_stack(argc, kern_stack_argvec);

	/* Start the task */
	sfree(kern_stack_args, NUM_USER_ARGS * USER_STR_LEN);
	free_pd_memory(old_pd);
	sfree(old_pd, PAGE_SIZE);
	task_start(tid, (uint32_t)esp, se_hdr.e_entry);

	panic("execute_user_program does not return");

	/* Swap back to old pd and clean up */
cleanup_w_pd:
	new_pd = swap_task_pd(old_pd);
	free_pd_memory(new_pd);

cleanup:
	sfree(kern_stack_args, NUM_USER_ARGS * USER_STR_LEN);
	return -1;
}

/** @brief Loads an initial user program such as 'idle' or 'init' and makes
 *         it runnable so that on the next context switch it will run and go
 *         from kernel mode to user mode to run that task
 *
 *  @pre fname, argc, argv must be validated and non-malicious, in kernel
 *       memory
 *
 *  @param fname Executable name
 *  @param argc Argument count
 *  @param argv Argument vector
 */
int
load_initial_user_program( char *fname, int argc, char **argv )
{
	/* Load user program information */
	simple_elf_t se_hdr;
	if (load_user_program_info(&se_hdr, fname) < 0) {
		return -1;
	}
    uint32_t pid, tid;

	if (create_task(&pid, &tid, &se_hdr) < 0)
		return -1;

	set_task_name(find_pcb(pid), fname);

	if (register_with_simics(tid, fname) < 0) {
		return -1;
	}
	/* If this is the init task, let the world know */
	register_if_init_task(fname, pid);

	/* Update page directory, enable VM if necessary */
	if (activate_task_memory(pid) < 0) {
		return -1;
	}
	uint32_t stack_lo = UINT32_MAX - USER_THREAD_STACK_SIZE + 1;
	if (_new_pages((uint32_t *) stack_lo, USER_THREAD_STACK_SIZE) < 0) {
		return -1;
	}


    if (transplant_program_memory(&se_hdr) < 0) {
		return -1;
	}
    uint32_t *esp = configure_stack(argc, argv);

	tcb_t *tcb = find_tcb(tid);
	if (configure_initial_task_stack(tcb, (uint32_t) esp, se_hdr.e_entry,
		get_tcb_pd(tcb)) < 0) {
		return -1;
	}
	assert(is_valid_pd(get_tcb_pd(find_tcb(tid))));

	/* Calls make_thread_runnable() */
	switch_safe_make_thread_runnable(tcb);
	return 0;
}

/** @brief Sets up the kernel thread stack for an initial task such as
 *         'init' or 'idle'
 *
 *  @pre Kernel must not have mode switched to user mode before to get the
 *       correct user eflags
 *  @pre Interrupts must be disabled
 */
int
configure_initial_task_stack( tcb_t *tcbp, uint32_t user_esp,
                              uint32_t entry_point, void *user_pd )
{
	/* Basic argument validation */
	if (!tcbp)
		return -1;
	if (!STACK_ALIGNED(tcbp->kernel_stack_hi))
		return -1;
	if (!STACK_ALIGNED(user_esp))
		return -1;
	if (entry_point < USER_MEM_START)
		return -1;
	if (!user_pd)
		return -1;
	if (!PAGE_ALIGNED(user_pd))
		return -1;

	/* Begin kernel thread stack setup */
	uint32_t *kernel_esp = tcbp->kernel_stack_hi;
	uint32_t kernel_ebp = (uint32_t) kernel_esp;

	/* Set up kernel thread stack for iret_travel */
 	*(--kernel_esp) = SEGSEL_USER_DS;
	log_warn("configure_initial_task_stack "
		"SEGSEL_USER_DS:%p at %p", (uint32_t *) SEGSEL_USER_DS, kernel_esp);

 	*(--kernel_esp) = user_esp;
	log_warn("configure_initial_task_stack "
		"user_esp:%p at %p", (uint32_t *) user_esp, kernel_esp);

 	*(--kernel_esp) = get_user_eflags();
	log_warn("configure_initial_task_stack "
		"user_eflags:%08lx at %p", get_user_eflags(), kernel_esp);

 	*(--kernel_esp) = SEGSEL_USER_CS;
	log_warn("configure_initial_task_stack "
		"SEGSEL_USER_CS:%08lx at %p",SEGSEL_USER_CS, kernel_esp);

 	*(--kernel_esp) = entry_point;
	log_warn("configure_initial_task_stack "
		"entry_point:%08lx at %p", entry_point, kernel_esp);

	--kernel_esp; /* simulate an iret_travel call's return address */
	*(--kernel_esp) = (uint32_t) iret_travel;
	log_warn("configure_initial_task_stack "
		"iret_tracel:%p at %p", iret_travel, kernel_esp);


	/* Set up kernel thread stack for context switch */
	uint32_t dummy_eax, dummy_ebx, dummy_ecx, dummy_edx, dummy_edi, dummy_esi;
	dummy_eax = dummy_ebx = dummy_ecx = dummy_edx = dummy_edi = dummy_esi = 0;

	*(--kernel_esp) = kernel_ebp;
	log_warn("configure_initial_task_stack "
		"ebp:%x at %p", kernel_ebp, kernel_esp);

	*(--kernel_esp) = dummy_eax;
	log_warn("configure_initial_task_stack "
		"dummy_eax:%x at %p", dummy_eax, kernel_esp);

	*(--kernel_esp) = dummy_ebx;
	log_warn("configure_initial_task_stack "
		"dummy_ebx:%x at %p", dummy_ebx, kernel_esp);

	*(--kernel_esp) = dummy_ecx;
	log_warn("configure_initial_task_stack "
		"dummy_ecx:%x at %p", dummy_ecx, kernel_esp);

	*(--kernel_esp) = dummy_edx;
	log_warn("configure_initial_task_stack "
		"dummy_edx:%x at %p", dummy_edx, kernel_esp);

	*(--kernel_esp) = dummy_edi;
	log_warn("configure_initial_task_stack "
		"dummy_edi:%x at %p", dummy_edi, kernel_esp);

	*(--kernel_esp) = dummy_esi;
	log_warn("configure_initial_task_stack "
		"dummy_esi:%x at %p", dummy_esi, kernel_esp);

	*(--kernel_esp) = get_cr0();
	log_warn("configure_initial_task_stack "
		"cr0:%p at %p", get_cr0(), kernel_esp);

	*(--kernel_esp) = (uint32_t) user_pd;
	log_warn("configure_initial_task_stack "
		"user_pd:%p at %p", user_pd, kernel_esp);


	/* Update tcbp esp */
	tcbp->kernel_esp = kernel_esp;

	return 0;
}




/** @brief Registers a task with simics if DEBUG flag is set, else this
 *         function is a no-op
 *
 *  @param tid Thread ID of first thread of task
 *  @param fname Task executable name
 *  @return 0 on success, -1 on error
 */
int
register_with_simics( uint32_t tid, char *fname )
{
#ifdef DEBUG
	if (tid == 0)
		return -1;

	if (!fname)
		return -1;

	tcb_t *tcb = find_tcb(tid);
	if (!tcb)
		return -1;

	uint32_t **pd = get_tcb_pd(tcb);
	if (!pd)
		return -1;

	/* Register this task's new binary with simics */
	sim_reg_process(pd, fname);
#endif
	return 0;
}

/* @brief Loads user program information into a simple_elf_t struct
 *
 * @param se_hdr Pointer to simple_elf_t struct to be filled
 * @return 0 on success, -1 on error
 */
int
load_user_program_info(simple_elf_t *se_hdrp, char *fname)
{
	if (!se_hdrp)
		return -1;

	if (elf_check_header(fname) == ELF_NOTELF)
		return -1;

    if (elf_load_helper(se_hdrp, fname) == ELF_NOTELF)
		return -1;

	return 0;
}
