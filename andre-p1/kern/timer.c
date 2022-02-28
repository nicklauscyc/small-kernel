/** @file timer.c
 *  @brief A timer handler.
 *
 *  Includes functions for setting up and handling timer interrupts.
 *  Interrupts are set to a rate of 100Hz (once every 10 ms).
 *
 *  @author Andre Nascimento (anascime)
 *  @bug No known bugs.
 *  */

#include <handler_wrapper.h>   /* asm_timer_wrapper */
#include <handlers.h>          /* register_handler */

#include <asm.h>               /* out_b */
#include <timer_defines.h>     /* TIMER_IDT_ENTRY, TIMER_RATE, TIMER_IO_PORT */
#include <interrupt_defines.h> /* INT_CTL_PORT, INT_ACK_CURRENT */
#include <stdint.h>            /* uint16_t */

/** @brief How often timer interrupts should be sent, in hz. */
#define DESIRED_TIMER_RATE 100

/** @brief Get 8 least significant bits in x. */
#define LSB(x) ((long unsigned int)x & 0xFF)

/** @brief Get 8 most significant bits in x. */
#define MSB(x) (((long unsigned int)x >> 8) & 0xFF)

/** @brief Function to be called by asm_timer_wrapper */
void (*timer_tickback_fn)();

/** @brief Number of ticks since handler was registered. */
unsigned int ticks;

/** @brief Handler for timer interrupts. Should only be called
 *         through register preserving asm wrapper.
 *
 *  @return Void.
 * */
void timer_handler() {
    /* Call user tickback function with new tick count. */
    timer_tickback_fn(++ticks);

    /* Let PIC interrupt has been handled. */
    outb(INT_CTL_PORT, INT_ACK_CURRENT);
}

/** @brief Sets up timer interrupts and the timer handler.
 *
 *  @param tickback Function to be called by timer_handler every
 *                  timer interrupt.
 *
 *  @return Void.
 *  */
void setup_timer(void (*tickback)(unsigned int)) {
    /* Set up timer_handler. */
    timer_tickback_fn = tickback;
    ticks = 0;

    register_handler(asm_timer_wrapper, TIMER_IDT_ENTRY);

    /* Configure timer. */
    outb(TIMER_MODE_IO_PORT, TIMER_SQUARE_WAVE);
    uint16_t wait_cycles = TIMER_RATE / DESIRED_TIMER_RATE;
    outb(TIMER_PERIOD_IO_PORT, LSB(wait_cycles)); /* Send lsb first... */
    outb(TIMER_PERIOD_IO_PORT, MSB(wait_cycles)); /* then msb. */
}
