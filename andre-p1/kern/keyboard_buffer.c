/** @file keyboard_buffer.c
 *  @brief A circular keyboard buffer.
 *
 *  The circular buffer behaves much like a queue.
 *  Data can be enqueued so long as the buffer is not full.
 *  And if it is, that data is silently ignored.
 *
 *  The dequeue operation can be safely interrupted, at any time, by
 *  one or multiple calls to enqueue. This enables the use of enqueue
 *  in an interrupt handler.
 *
 *  @author Andre Nascimento (anascime)
 *  @bug No known bugs.
 * */

#include <keyboard_buffer.h>

#include <assert.h> /* assert() */
#include <malloc.h> /* malloc() */
#include <stdint.h> /* uint8_t */
#include <asm.h>    /* outb(), inb() */

/** @brief Sets up new keyboard buffer.
 *
 *  Perhaps unusually, instead of allocating a buffer,
 *  it takes one as an argument.
 *
 *  @param out A pointer to the buffer being set up.
 *  @param bytes A pointer to a memory region to be used by the keyboard buffer.
 *  @param size The size of the bytes buffer.
 *
 *  @return 0 if it succeeded, -1 otherwise.
 *  */
int kb_setup(keyboard_buffer_t *out, uint8_t *bytes, uint32_t size) {
    if (!bytes || !out) return -1;
    out->bytes = bytes;
    out->size = size;
    out->start = 0;
    out->end = 0;
    return 0;
}

/** @brief Checks whether keyboard buffer is empty.
 *
 *  @param in The keyboard buffer to check.
 *
 *  @return 1 if empty, 0 otherwise.
 *  */
int kb_empty(keyboard_buffer_t *in) {
    assert(in);
    return in->start == in->end;
}

/** @brief Checks whether the keyboard buffer is full.
 *
 *  @param in The keyboard buffer to check.
 *
 *  @return 1 if full, 0 otherwise.
 *  */
int kb_full(keyboard_buffer_t *in) {
    assert(in);
    return (in->end + 1) % in->size == in->start;
}

/** @brief Dequeue element from the buffer.
 *         Requires kb_empty to be false.
 *
 *  @param in The keyboard buffer from which to dequeue
 *
 *  @return The dequeued value.
 *  */
uint8_t kb_dequeue(keyboard_buffer_t *in) {
    assert(in);
    assert(in->start < in->size && in->end < in->size);
    assert(!kb_empty(in));
    uint8_t result = in->bytes[in->start];
    in->start = (in->start + 1) % in->size;
    return result;
}

/** @brief Attempts to enqueue value. However, if the buffer
 *         is full, value is just ignored.
 *
 *  @param in The keyboard in which to attempt enqueuing.
 *  @param val The value to attempt enqueuing.
 *
 *  @return Void.
 *  */
void kb_try_enqueue(keyboard_buffer_t *in, uint8_t val) {
    assert(in->start < in->size && in->end < in->size);
    if (kb_full(in)) return;
    in->bytes[in->end] = val;
    in->end = (in->end + 1) % in->size;
}
