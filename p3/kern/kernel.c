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

#include <common_kern.h>

/* libc includes. */
#include <stdio.h>
#include <simics.h>                 /* lprintf() */

/* multiboot header file */
#include <multiboot.h>              /* boot_info */

/* x86 specific includes */
#include <x86/asm.h>                /* enable_interrupts() */

#include <loader.h>  /* execute_user_program() */
#include <console.h> /* clear_console() */

volatile static int __kernel_all_done = 0;

//int readline(char *buf, int len);

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

    /* When kernel_main() begins, interrupts are DISABLED.
     * You should delete this comment, and enable them --
     * when you are ready.
     */

    lprintf("Hello from a brand new kernel! Executing gettid_test... ");

	clear_console();

    /* TODO: Start init, idle and shell. For now
     * hardcode execution of gettid test binary. */
    execute_user_program("gettid_test", argc, argv);


    while (!__kernel_all_done) {
        continue;
    }

    return 0;
}
