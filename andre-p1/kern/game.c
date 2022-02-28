/** @file game.c
 *  @brief A kernel with timer, keyboard, console support
 *
 *  This file contains the kernel's main() function.
 *
 *  It sets up the drivers and starts the game.
 *
 *  @author Michael Berman (mberman)
 *  @author Andre Nascimento (anascime)
 *  @bug No known bugs.
 */

#include <p1kern.h>
#include "inc/handlers.h"

/* libc includes. */
#include <stdio.h>
#include <malloc.h>

/* multiboot header file */
#include <multiboot.h>              /* boot_info */

/* x86 specific includes */
#include <x86/asm.h>                /* enable_interrupts() */

#include <string.h>

void tick(unsigned int numTicks);

volatile static int __kernel_all_done = 0;

/** @brief Kernel entrypoint.
 *
 *  This is the entrypoint for the kernel.  It simply sets up the
 *  drivers and echoes whatever the user types.
 *
 * @return Does not return
 */
int kernel_main(mbinfo_t *mbinfo, int argc, char **argv, char **envp) {
    /*
     * Initialize device-driver library.
     */
    handler_install(tick);

    enable_interrupts();

    set_term_color(FGND_WHITE);

    clear_console();

    show_cursor();

    char buf[1024];
    while (1) {
        int bytes = readline(buf, 1024);
        putbytes(buf, bytes);
    }

    disable_interrupts();

    while (!__kernel_all_done) {
        continue;
    }

    return 0;
}

/** @brief Tick function, to be called by the timer interrupt handler
 *
 *  In a real game, this function would perform processing which
 *  should be invoked by timer interrupts.
 *
 **/
void tick(unsigned int numTicks)
{
}
