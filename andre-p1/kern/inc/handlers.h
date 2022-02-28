/** @file handlers.h
 *  @brief Function prototypes for handlers.
 *
 *  @author Andre Nascimento (anascime)
 *  @bug No known bugs.
 *  */

#ifndef HANDLERS_H_
#define HANDLERS_H_

typedef struct {
    unsigned int offset_lsb : 16;
    unsigned int segment_selector : 16;
    unsigned int  : 5;
    unsigned int zeroes : 3;
    unsigned int entry_type : 5;
    unsigned int DPL : 2;
    unsigned int P : 1;
    unsigned int offset_msb : 16;
} idt_entry_t;

void register_handler(void *addr, long unsigned int idt_index);

int handler_install(void (*tickback)(unsigned int));

#endif /* HANDLERS_H_ */
