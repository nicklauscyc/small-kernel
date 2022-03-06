/** @file install_handler.c
 *  @brief Contains functions that install the timer and keyboard handlers
 *
 *  @author Nicklaus Choo (nchoo)
 *  @bug No known bugs.
 */

#include <x86/asm.h> /* idt_base() */
#include <assert.h> /* assert() */
#include <x86/interrupt_defines.h>
#include <x86/timer_defines.h> /* TIMER_IDT_ENTRY */
#include <x86/seg.h> /* SEGSEL_KERNEL_CS */
#include <x86/keyhelp.h>
#include <stddef.h> /* NULL */
#include <simics.h> /* lprintf */
#include "./asm_interrupt_handler.h" /* call_timer_int_handler(),
                                        call_keybd_int_handler() */
#include <timer_driver.h> /* init_timer() */
#include <keybd_driver.h> /* init_keybd() */
#include <install_handler.h>

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
#define D16                 0x00000700
#define D32                 0x00000F00
#define RESERVED_UPPER_MASK 0x0000000F

/* Hander installation error codes */
#define E_NO_INSTALL_KEYBOARD_HANDLER -2
#define E_NO_INSTALL_TIMER_HANDLER -3

/* Type of assembly wrapper for C interrupt handler functions */
typedef void asm_wrapper_t(void);

/*********************************************************************/
/*                                                                   */
/* Internal helper functions                                         */
/*                                                                   */
/*********************************************************************/

/** @brief Installs an interrupt handler at idt_entry for timer and keyboard
 *         interrupt handlers.
 *
 *  If a tickback function pointer is provided, init_timer() will be called.
 *  Else it is assumed that init_keybd() should be called instead, since there
 *  are only 2 possible idt_entry values to accept.
 *
 *  Furthermore, when casting from pointer types to integer types to pack
 *  bits into the trap gate data structure, the pointer is first cast to
 *  unsigned long then unsigned int, therefore there is no room for undefined
 *  behavior.
 *
 *  @param idt_entry Index into IDT table.
 *  @param asm_wrapper Assembly wrapper to call C interrupt handler
 *  @param tickback Application provided callback function for timer interrupts.
 *  @return 0 on success, -1 on error.
 */
int handler_install_in_idt(int idt_entry, asm_wrapper_t *asm_wrapper,
                           void (*tickback)(unsigned int)) {

  if (asm_wrapper == NULL) return -1;

  /* Only when installing the timer handler do we have a non-NULL tickback */
  if (tickback != NULL) {
    init_timer(tickback);
  } else {
    init_keybd();
  }
  /* Get address of trap gate for timer */
  void *idt_base_addr = idt_base();
  void *idt_entry_addr = idt_base_addr + (idt_entry * BYTES_PER_GATE);

  /* Exact offsets of each 32-bit word in the trap gate */
  unsigned int *idt_entry_addr_lower = idt_entry_addr;
  unsigned int *idt_entry_addr_upper = idt_entry_addr + (BYTES_PER_GATE / 2);
  if (idt_entry_addr_lower == NULL || idt_entry_addr_upper == NULL) return -1;

  /* Construct data for upper 32-bit word with necessary flags */
  unsigned int data_upper = 0;
  unsigned int offset_upper =
    ((unsigned int) (unsigned long) asm_wrapper) & OFFSET_UPPER_MASK;
  data_upper = offset_upper | PRESENT | DPL_0 | D32;

  /* Zero all bits that are not reserved, pack data into upper 32-bit word */
  *idt_entry_addr_upper = *idt_entry_addr_upper & RESERVED_UPPER_MASK;
  *idt_entry_addr_upper = *idt_entry_addr_upper | data_upper;

  /* Construct data for lower 32-bit word with necessary flags */
  unsigned int data_lower = 0;
  unsigned int offset_lower =
    ((unsigned int) (unsigned long) asm_wrapper) & OFFSET_LOWER_MASK;
  data_lower = (SEGSEL_KERNEL_CS << (2 * BYTE_LEN)) | offset_lower;

  /* Pack data into lower 32-bit word */
  *idt_entry_addr_lower = data_lower;

  /* This should always be the case after writing to IDT */
  assert(*idt_entry_addr_lower == data_lower);
  assert(*idt_entry_addr_upper == data_upper);

  return 0;
}

/*********************************************************************/
/*                                                                   */
/* Interface for device-driver initialization and timer callback     */
/*                                                                   */
/*********************************************************************/

/** @brief The driver-library initialization function
 *
 *   Installs the timer and keyboard interrupt handler.
 *   NOTE: handler_install should ONLY install and activate the
 *   handlers; any application-specific initialization should
 *   take place elsewhere.
 *   --
 *   After installing both the timer and keyboard handler successfully,
 *   interrupts are enabled.
 *   --
 *   @param tickback Pointer to clock-tick callback function
 *   @return A negative error code on error, or 0 on success
 **/
int handler_install(void (*tickback)(unsigned int)) {

  /* While interrupt handlers are not set up, disable interrupts */
  disable_interrupts();

  /* Initialize and install timer handler */
  int res = handler_install_in_idt(TIMER_IDT_ENTRY, call_timer_int_handler,
                                   tickback);
  if (res < 0) return E_NO_INSTALL_TIMER_HANDLER;

  /* Initialize and install keyboard handler */
  res = handler_install_in_idt(KEY_IDT_ENTRY, call_keybd_int_handler, NULL);
  if (res < 0) return E_NO_INSTALL_KEYBOARD_HANDLER;

  /* Interrupt handlers successfully installed, enable interrupts */
  enable_interrupts();
  return 0;
}

