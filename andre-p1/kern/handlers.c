/** @file handlers.c
 *  @brief Facilities for installing handlers in the IDT.
 *
 *  @author Andre Nascimento (anascime)
 *  @bug No known bugs.
 */

/* -- Includes -- */

#include <handlers.h>
#include <keyboard.h> /* setup_keyboard() */
#include <timer.h>    /* setup_timer() */

#include <p1kern.h>
#include <asm.h>    /* idt_base() */
#include <seg.h>    /* SEGSEL_KERNEL_CS */
#include <simics.h> /* lprintf() */

/** @brief Get 16 least significant bits of x. */
#define LSB(x) ((long unsigned int)x & 0xFFFF)
/** @brief Get 16 most significant bits of x. */
#define MSB(x) (((long unsigned int)x >> 16) & 0xFFFF)
/** @brief Trap gate and D flags for IDT entry. */
#define TRAP_GATE_TYPE 0xF

/** @brief Register handler in Interrupt Descriptor Table.
 *
 *  @param addr Address of handler function, of the form void fn(void).
 *  @param idt_index Index into IDT where handler should be installed.
 *
 *  @return Void.
 *  */
void register_handler(void *addr, long unsigned int idt_index) {
    /* Fill in IDT entry. */
    idt_entry_t idt_entry = {
        .offset_lsb       = LSB(addr),
        .segment_selector = (long unsigned int)SEGSEL_KERNEL_CS,
        .zeroes           = 0,
        .entry_type       = (long unsigned int)TRAP_GATE_TYPE,
        .DPL              = 0,
        .P                = 1,
        .offset_msb       = MSB(addr),
    };

    /* Get timer entry and update its value. */
    ((idt_entry_t *)idt_base())[idt_index] = idt_entry;
}

/** @brief Request timer and keyboard modules to register
 *         their handlers through register_handler.
 *
 *  @param tickback Function to be called every timer tick,
 *                  with total number of ticks as argument.
 *
 *  @return Void. */
int handler_install(void (*tickback)(unsigned int)) {
    setup_timer(tickback);
    setup_keyboard();
    return 0;
}

