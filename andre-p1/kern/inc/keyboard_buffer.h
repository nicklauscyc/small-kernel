#ifndef KEYBOARD_BUFFER_H_
#define KEYBOARD_BUFFER_H_

#include <stdint.h>        /* uint8_t */

typedef struct {
    uint8_t *bytes;
    uint32_t size;
    uint32_t start;
    uint32_t end;
} keyboard_buffer_t;

int kb_setup(keyboard_buffer_t *out, uint8_t *bytes, uint32_t size);

int kb_empty(keyboard_buffer_t *in);

int kb_full(keyboard_buffer_t *in);

uint8_t kb_dequeue(keyboard_buffer_t *in);

void kb_try_enqueue(keyboard_buffer_t *in, uint8_t val);

#endif /* KEYBOARD_BUFFER_H_ */

