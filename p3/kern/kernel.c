/** @file kernel.c
 *  @brief An initial kernel.c
 *
 *  You should initialize things in kernel_main(),
 *  and then run stuff.
 *
 *  @author Harry Q. Bovik (hqbovik)
 *  @author Fred Hacker (fhacker)
 *  @bug No known bugs.
 */

#include <logger.h> /* log(), log_med(), log_hi() */
#include <install_handler.h> /* handler_install() */
#include <common_kern.h>

/* libc includes. */
#include <stdio.h>
#include <simics.h>                 /* lprintf() */

/* multiboot header file */
#include <multiboot.h>              /* boot_info */

/* x86 specific includes */
#include <x86/asm.h>                /* enable_interrupts() */

#include <x86/cr.h> /* get_cr3() */

#include <exec2obj.h> /* MAX_EXECNAME_LEN */

#include <loader.h>     /* execute_user_program() */
#include <console.h>    /* clear_console(), putbytes() */
#include <keybd_driver.h> /* readline() */
#include <malloc.h> /*malloc() */

#include <physalloc.h> /* test_physalloc() */
#include <scheduler.h> /* run_next_tcb() */
volatile static int __kernel_all_done = 0;


/* Level of logging, set to 4 to turn logging off,
 * 1 to print logs for log priorities lo, med, hi
 * 2 to print logs for log priorities med, hi
 * 3 to print logs for log priorities hi
 *
 * defining the NDEBUG flag will also turn logging off
 */
int log_level = 2;

void tick(unsigned int numTicks) {
	scheduler_on_tick(numTicks);
}

/** @brief Kernel entrypoint.
 *
 *  This is the entrypoint for the kernel.
 *
 * @return Does not return
 */
int
kernel_main( mbinfo_t *mbinfo, int argc, char **argv, char **envp )
{
    /* FIXME: What to do with mbinfo and envp? */
    (void)mbinfo;
    (void)envp;

	/* initialize handlers and enable interrupts */
	if (handler_install(tick) < 0) {
		panic("cannot install handlers");
	}
	enable_interrupts();

	clear_console();

    /* TODO
     * When kernel_main() begins, interrupts are DISABLED.
     * You should delete this comment, and enable them --
     * when you are ready.
     */

	//test_physalloc();

    lprintf( "Hello from a brand new kernel!" );
    //TODO: Print programs using elf->e_name
	printf("executable user programs:\n");
	printf("loader_test1\n");
	printf("loader_test2\n");
	printf("getpid_test1\n");
	printf("fork_test1\n");

	log("this is DEBUG");
	log_info("this is INFO");
	log_warn("this is WARN");

    while (!__kernel_all_done) {

        //char* s = "context_switch_test";
        //char *user_argv = (char *)s;
        //execute_user_program(s, 1, &user_argv);
        //break;

        //TODO original code is below. code above this is for running
        //getpid_test1 pronto with no user input (for testing)

        int n = MAX_EXECNAME_LEN;
        char s[n];

        /* Display prompt */
        printf("pebbles>");
        int res = readline(s, n);

        if (res == n)
            continue; /* Executable name too large */

        /* Swap \n returned by readline for null-terminator */
        s[res - 1] = '\0';

        lprintf("Executing: %s", s);

        char *user_argv = (char *)s;

        execute_user_program(s, 1, &user_argv);
    }

    return 0;
}
