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

/* Think about where this declaration
 * should be... probably not here!
 */
extern int scheduler_init;

void tick(unsigned int numTicks) {
	if (numTicks % 100 == 0 && scheduler_init) {
		lprintf("ctx switchnumTicks: %d\n", numTicks);
		run_next_tcb();
	}
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

	test_physalloc();

    lprintf( "Hello from a brand new kernel!" );
	putbytes("executable user programs:\n", 26);
	putbytes("loader_test1\n", 13);
	putbytes("loader_test2\n", 13);
	putbytes("getpid_test1\n", 13);
	putbytes("fork_test1\n", 13);

	///* On kernel_main() entry, all control registers are 0 */
	//lprintf("cr1: %p", (void *) get_cr3());
	//lprintf("cr2: %p", (void *) get_cr3());
	//lprintf("cr3: %p", (void *) get_cr3());
	//lprintf("cr4: %p", (void *) get_cr3());
	//char * nullp = 0;
	//lprintf("garbage at address 0x0:%d", *nullp);
    //lprintf("&nullp:%p", &nullp);
	void *p = malloc(8);
	(void) p;

    while (!__kernel_all_done) {

		//char* s = "getpid_test1";
        //char *user_argv = (char *)s;
		//execute_user_program(s, 1, &user_argv);

		//TODO original code is below. code above this is for running
		//getpid_test1 pronto with no user input (for testing)

		int n = MAX_EXECNAME_LEN;
     	char s[n];

	 	/* Display prompt */
     	putbytes("pebbles>",8);
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
