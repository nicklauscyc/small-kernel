
#ifndef _INSTALL_HANDLER_H_
#define _INSTALL_HANDLER_H_

/* Number of bits in a byte */
#define BYTE_LEN 8

/* Number of bytes that a trap gate occupies */
#define BYTES_PER_GATE 8

/* Mask for function handler address for upper bits */
#define OFFSET_UPPER_MASK   0xFFFF0000

/* Mask for function handler address for lower bits */
#define OFFSET_LOWER_MASK   0x0000FFFF

/* Trap gate flag masks */
#define PRESENT             0x00008000
#define DPL_0               0x00000000
#define DPL_3               0x00000003

#define D16                 0x00000700
#define D32                 0x00000F00
#define RESERVED_UPPER_MASK 0x0000000F

/* Hander installation error codes */
#define E_NO_INSTALL_KEYBOARD_HANDLER -2
#define E_NO_INSTALL_TIMER_HANDLER -3

/* Type of assembly wrapper for C interrupt handler functions */
typedef void asm_wrapper_t(void);

int handler_install(void (*tick)(unsigned int));

int install_handler_in_idt(int idt_entry, asm_wrapper_t *asm_wrapper, int dpl);


#endif
