/** @file console.c
 *  @brief A console driver.
 *
 *  Implements functions defined in console.h to enable
 *  control of the console. Communicates with video card
 *  through video-memory and CRTC to implement a cursor.
 *  Of note, characters and the cursor will not be visible
 *  unless a prior call to set_term_color is called.
 *
 *  @author Andre Nascimento (anascime)
 *  @bug No known bugs.
 * */

/* -- Includes -- */

#include <console.h>
#include <p1kern.h>

#include <ctype.h>      /* isprint() */
#include <string.h>     /* memset(), memmove() */
#include <assert.h>     /* assert(), affirm() */
#include <x86/asm.h>    /* outb() */

/** @brief Logical cursor type.
 * */
typedef struct {
	int row;
	int col;
	int hidden;
} cursor_t;

/** @brief Represents position of logical cursor. Should only
 *         be modified by set_logical_cursor.
 * */
cursor_t cursor = {
    .row = 0,
    .col = 0,
    .hidden = 1
};

/** @brief Color to be used for future characters printed in console.
 * */
int console_color = 0;

/* ----- Helper functions ----- */

/** @brief Checks whether cursor data is valid.
 *
 *  @return Whether the cursor is valid.
 * */
int is_cursor_valid() {
    if (!(0 <= cursor.row && cursor.row < CONSOLE_HEIGHT)) return 0;
    if (!(0 <= cursor.col && cursor.col < CONSOLE_WIDTH)) return 0;
    if (cursor.hidden != 0 && cursor.hidden != 1) return 0;
    return 1;
}

/** @brief Checks if color is a valid combination of foreground
 *         and background colors.
 *  @param Color.
 *  @return Whether the color is valid.
 *  */
int is_color_valid(int color) {
    return 0x0 <= color && color <= 0x8F;
}

/** @brief Updates logical cursor, modifying cursor_info.
 *         If necessary, updates hardware cursor.
 *
 *  @param row New row for the logical/hw cursor.
 *  @param col New col for the logical/hw cursor.
 *  @param hidden Whether to hide the hardware cursor.
 *
 *  @return Void.
 *  */
void set_logical_cursor(int row, int col, int hidden) {
    /* Check whether we should even bother the CRTC*/
    int update_hw = 0;
    if (hidden && !cursor.hidden) update_hw = 1;
    if (!hidden && (cursor.hidden || cursor.row != row || cursor.col != col))
        update_hw = 1;

    cursor.row = row;
    cursor.col = col;
    cursor.hidden = hidden;
    assert(is_cursor_valid());

    if (update_hw) {
        /* Push cursor offscreen */
        if (hidden) {
            row = CONSOLE_HEIGHT;
            col = CONSOLE_WIDTH;
        }
        int val = row * CONSOLE_WIDTH + col;
        uint8_t lsb_val = val & 0xFF;
        uint8_t msb_val = (val >> 8) & 0xFF;

        /* Set least significant bits */
        outb(CRTC_IDX_REG, CRTC_CURSOR_LSB_IDX);
        outb(CRTC_DATA_REG, lsb_val);

        /* Set most signficant bits*/
        outb(CRTC_IDX_REG, CRTC_CURSOR_MSB_IDX);
        outb(CRTC_DATA_REG, msb_val);
    }
}

/** @brief Clear some lines of console at a given point.
 *
 *  All lines to be cleared must be within video memory.
 *
 *  @param from Where to start.
 *  @param lines How many lines to clear.
 *
 *  @return Void.
 *  */
void clear(char *from, int lines) {
    assert(from >= (char *)CONSOLE_MEM_BASE &&
            from + lines * CONSOLE_WIDTH * 2 <=
            (char *)CONSOLE_MEM_BASE + (CONSOLE_WIDTH * CONSOLE_HEIGHT * 2));

    for (int i=0; i < lines * CONSOLE_WIDTH; ++i) {
        from[i*2    ] = ' '; /* Empty character */
        from[i*2 + 1] = console_color;
    }
}

/** @brief Scroll console down.
 *
 *  @param lines Number of lines to scroll.
 *  @return Void.
 *  */
void scroll(int lines) {
    assert(lines >= 0);
    lines = lines >= CONSOLE_HEIGHT ? CONSOLE_HEIGHT : lines;

    /* Get what will be the new start of console memory */
    char *new_start = (char *)(CONSOLE_MEM_BASE + lines * CONSOLE_WIDTH * 2);

    /* Move that memory up */
    memmove((char *)CONSOLE_MEM_BASE, new_start, (CONSOLE_HEIGHT - lines) * CONSOLE_WIDTH * 2);

    /* Clear remaining garbage at the bottom */
    char *garbage_start = (char *)(CONSOLE_MEM_BASE + (CONSOLE_HEIGHT - lines) * CONSOLE_WIDTH * 2);
    clear(garbage_start, lines);
}

/** @brief Moves cursor to new_line, scrolling if necessary.
 *
 *  @return Void.
 *  */
void new_line() {
    if (cursor.row == CONSOLE_HEIGHT - 1) {
        scroll(1);
        set_logical_cursor(cursor.row, 0, cursor.hidden);
    } else {
        set_logical_cursor(cursor.row + 1, 0, cursor.hidden);
    }
}

/* ----- Public functions ----- */

/** @brief Writes 'ch' at logical cursor position.
 *
 *  Handles backspace, carriage return and new lines.
 *  Also updates cursor position.
 *
 *  @param ch Character to print.
 *  @return Void.
 *  */
int putbyte(char ch) {
    assert(is_cursor_valid());
    assert(is_color_valid(console_color));
    if (ch == '\b') {
        if (cursor.col == 0) { /* backspace back into last line */
            if (cursor.row == 0) return ch;
            draw_char(cursor.row - 1, CONSOLE_WIDTH - 1, ' ', console_color);
            set_logical_cursor(cursor.row - 1, CONSOLE_WIDTH - 1, cursor.hidden);
        } else {
            draw_char(cursor.row, cursor.col - 1, ' ', console_color);
            set_logical_cursor(cursor.row, cursor.col - 1, cursor.hidden);
        }
    } else if (ch == '\r') {
        set_logical_cursor(cursor.row, 0, cursor.hidden);
    } else if (ch == '\n') {
        new_line();
    } else {
        draw_char(cursor.row, cursor.col, ch, console_color);
        if (cursor.col == CONSOLE_WIDTH - 1) {
            new_line();
        } else {
            set_logical_cursor(cursor.row, cursor.col + 1, cursor.hidden);
        }
    }
    return ch;
}

/** @brief Writes 's' to console at logical cursor position.
 *         Same semantics as putbyte for each character being printed.
 *
 *  Prints until null termination character in s or up to len.
 *
 *  @param s The string to be printed.
 *  @param len Max length to print.
 *
 *  @return Void.
 *  */
void putbytes(const char *s, int len) {
    if (!s) return;
    while (*s && len-- > 0) {
        putbyte(*s);
        s++;
    }
}

/** @brief Sets term color.
 *
 *  @param color New terminal color.
 *
 *  @return 0 on success, -1 if color is invalid.
 *  */
int set_term_color(int color) {
    if (!is_color_valid(color)) return -1;
    console_color = color;
    return 0;
}

/** @brief Gets term color.
 *
 *  @param color Pointer to where color is stored.
 *
 *  @return Void.
 * */
void get_term_color(int *color) {
  *color = console_color;
}

/** @brief Sets logical cursor position, if valid.
 *
 *  @param row New row for logical cursor.
 *  @param col New col for logical cursor.
 *
 *  @return 0 if the cursor was succesfully set, -1 otherwise.
 *  */
int set_cursor(int row, int col) {
    if (!(0 <= row && row < CONSOLE_HEIGHT)) return -1;
    if (!(0 <= col && col < CONSOLE_WIDTH)) return -1;
    set_logical_cursor(row, col, cursor.hidden);
    return 0;
}

/** @brief Gets cursor position.
 *
 *  @param row Where to store cursor row.
 *  @param col Where to store cursor col
 *
 *  @return Void.
 *  */
void get_cursor(int *row, int *col) {
    if (!row || !col) return;
    *row = cursor.row;
    *col = cursor.col;
}

/** @brief Hides cursor.
 *
 *  @return Void.
 *  */
void hide_cursor(void) {
    set_logical_cursor(cursor.row, cursor.col, 1);
}

/** @brief Shows cursor.
 *
 *  @return Void.
 *  */
void show_cursor(void) {
    set_logical_cursor(cursor.row, cursor.col, 0);
}

/** @brief Clear console, setting background to current term_color.
 *
 *  @return Void.
 *  */
void clear_console(void) {
    clear((char *)CONSOLE_MEM_BASE, CONSOLE_HEIGHT);
    set_logical_cursor(0, 0, cursor.hidden);
}

/** @brief Draw character to console with specified color.
 *
 *  Only prints character if row and column are within range
 *  of console and if ch is printable.
 *
 *  @param row Row in which to draw character.
 *  @param col Column in which to draw character.
 *  @param ch Character to draw.
 *  @param color Color to draw character as.
 *  */
void draw_char(int row, int col, int ch, int color) {
    if (!(0 <= row && row < CONSOLE_HEIGHT)) return;
    if (!(0 <= col && col < CONSOLE_WIDTH)) return;
    if (!is_color_valid(color)) return;
    if (!isprint(ch)) return;

    char *at = (char *)(CONSOLE_MEM_BASE + 2 * (row * CONSOLE_WIDTH + col));
    *at = ch;
    *(at + 1) = color;
}

/** @brief Get character at specified console position.
 *
 *  @param row Character's row
 *  @param col Character's col
 *
 *  @return character, 0 if location is invalid. */
char get_char(int row, int col) {
    if (!(0 <= row && row < CONSOLE_HEIGHT)) return 0;
    if (!(0 <= col && col < CONSOLE_WIDTH)) return 0;
    return *(char *)(CONSOLE_MEM_BASE + 2 * (row * CONSOLE_WIDTH + col));
}

