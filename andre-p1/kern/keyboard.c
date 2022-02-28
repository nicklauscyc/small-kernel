/** @file keyboard.c
 *  @brief A keyboard driver.
 *
 *  Handles keyboard input by using a circular buffer. This
 *  buffer stores up to CONSOLE_WIDTH * CONSOLE_HEIGHT
 *  bytes of input and drops any further bytes (if input
 *  is not consumed by readchar and readline). This was chosen
 *  as it provides the most reasonable behavior for the user.
 *  If we instead dropped old bytes, the input provided to readchar
 *  and readline would be unpredictable to the user.
 *  We also don't increase the buffer size as
 *  CONSOLE_WIDTH * CONSOLE_HEIGHT is a reasonable, albeit heuristic,
 *  buffer for user input.
 *
 *  On interrupt safety: The keyboard buffer dequeue operation can be
 *  interrupted with calls to the enqueue operation while maintaining
 *  correctness. This means it is a suitable implementation for a
 *  single-threaded application and as such enable_interrupts() and
 *  disable_interrupts() are not needed.
 *
 *  @author Andre Nascimento (anascime)
 *  @bug No known bugs.
 *  */

#include <keyboard.h>
#include <keyboard_buffer.h>   /* kb_new, kb_empty, kb_enqueue, kb_dequeue */
#include <console.h>           /* putbyte, get_cursor_pos */
#include <handlers.h>          /* register_handler() */
#include <handler_wrapper.h>   /* asm_keyboard_wrapper */

#include <video_defines.h>     /* CONSOLE_HEIGHT, CONSOLE_WIDTH */
#include <timer_defines.h>     /* TIMER_IDT_ENTRY, TIMER_RATE, TIMER_IO_PORT */
#include <interrupt_defines.h> /* INT_CTL_PORT, INT_ACK_CURRENT */
#include <stdio.h>             /* printf() */
#include <keyhelp.h>           /* process_scancode(), KEY_IDT_ENTRY */
#include <stdint.h>            /* uint8_t */
#include <string.h>            /* memcpy() */
#include <stdlib.h>            /* malloc(), free() */
#include <asm.h>               /* inb(), outb() */

/** @brief Size of keyboard buffer */
#define BUFFER_SIZE CONSOLE_WIDTH * CONSOLE_HEIGHT

/** @brief Max of two numbers. */
#define MAX(x,y) (x) > (y) ? (x) : (y)

/** @brief Keyboard buffer where keyboard input is stored before being processed. */
keyboard_buffer_t kb_buffer;

/** @brief Memory for keyboard buffer */
uint8_t kb_bytes[BUFFER_SIZE];

/** @brief Handler called by keyboard interrupt.
 *
 *  Enqueues keyboard input into kb_buffer and acknowledges
 *  interrupt was received.
 *
 *  @return Void.
 *  */
void keyboard_handler() {
    /* Read keyboard port and try storing in kb_buffer */
    uint8_t val = inb(KEYBOARD_PORT);
    kb_try_enqueue(&kb_buffer, val);

    /* Acknowledge current interrupt */
    outb(INT_CTL_PORT, INT_ACK_CURRENT);
}

/** @brief Sets up keyboard buffer and interrupt handler.
 *
 *  @return Void.
 *  */
void setup_keyboard() {
    kb_setup(&kb_buffer, kb_bytes, BUFFER_SIZE);
    register_handler(asm_keyboard_wrapper, KEY_IDT_ENTRY);
}

/** @brief Reads 1 character of user input. If no inputs are buffered,
 *         either because no input has been made, or all have already been
 *         read, readchar returns -1.
 *
 *  Reads character from buffer and translates to printable character.
 *  Extended keys are dropped.
 *
 *  @return Void.
 *  */
int readchar() {
    if (kb_empty(&kb_buffer)) return -1;
    kh_type key = process_scancode(kb_dequeue(&kb_buffer));

    if (KH_HASDATA(key) && KH_ISMAKE(key)) {
        /* Ignore extended keys */
        if (KH_ISEXTENDED(key)) return -1;

        /* Return something reasonable for undefined chars */
        if (KH_GETCHAR(key) == KHE_UNDEFINED) return '?';

        return KH_GETCHAR(key);
    }
    return -1;
}

/** @brief Reads line of user input.
 *
 *  Reads until user inputs \n or until len, whichever is shorter.
 *  If len is larger than the screen size, then readline fails.
 *  Readline deals with carriage-return and backspace appropriately,
 *  without letting them waste buffer space. Furthermore, neither
 *  the carriage-return, nor backspace will ever take the cursor to
 *  text not inputted after a call to readline (user-prompts are safe!).
 *
 *  @param buf Buffer where to store user input.
 *  @param len Maximum number of bytes to store in buf.
 *
 *  @return 0 on success, -1 on failure.
 *  */
int readline(char *buf, int len) {
    /* If len is larger than the max number of characters that can
     * be on the screen, just return. */
    if (len <= 0 || len >= CONSOLE_WIDTH * CONSOLE_HEIGHT) return -1;

    char *temp_buf = malloc(sizeof(char) * len);
    int start_row, start_col, val, max_len, i = 0;
    int curr_row, curr_col;
    get_cursor(&start_row, &start_col);

    while(i < len) {
        val = readchar();
        if (val < 0) continue;

        /* Handle special keys */
        if (val == KHE_BACKSPACE) {
            if (i == 0) continue;
            /* Don't waste space in the buffer for backspace. */
            temp_buf[--i] = ' ';
            putbyte(val);
        } else if (val == '\r') {
            /* Instead of overwriting prompt, go to start of user input.
             * Also don't waste space. */
            get_cursor(&curr_row, &curr_col);
            if (curr_row == start_row) {
                i = 0;
                set_cursor(curr_row, start_col);
            } else {
                i -= curr_col;
                set_cursor(curr_row, 0);
            }
        } else {
            temp_buf[i++] = val;
            /* Store i in max_len so we know up to what length should
             * be copied to buffer. In case carriage-return is called,
             * i will be different than the number of bytes stored in buf. */
            max_len = i;
            putbyte(val);
        }

        if (val == KHE_ENTER) { // We're done
            break;
        }
    }

    memcpy(buf, temp_buf, MAX(i, max_len));

    free(temp_buf);

    return i;
}
