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

#include <install_thread_management_handlers.h> /* install_gettid_handler() */

/*********************************************************************/
/*                                                                   */
/* Internal helper functions                                         */
/*                                                                   */
/*********************************************************************/

/** @brief Installs an interrupt handler at idt_entry for gettid()
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
int
install_handler_in_idt(int idt_entry, asm_wrapper_t *asm_wrapper)
{
  if (!asm_wrapper) {
	  return -1;
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

/** @brief Install timer interrupt handler
 */
int
install_timer_handler(int idt_entry, asm_wrapper_t *asm_wrapper,
                      void (*tickback)(unsigned int))
{
	if (!asm_wrapper) {
		return -1;
	}
    init_timer(tickback);
	return install_handler_in_idt(idt_entry, asm_wrapper);
}

/** @brief Install keyboard interrupt handler
 */
int
install_keyboard_handler(int idt_entry, asm_wrapper_t *asm_wrapper)
{
	if (!asm_wrapper) {
		return -1;
	}
    init_keybd();
	return install_handler_in_idt(idt_entry, asm_wrapper);
}

/*********************************************************************/
/*                                                                   */
/* Interface for device-driver initialization and timer callback     */
/*                                                                   */
/*********************************************************************/

/** @brief Installs all interrupt handlers by calling the correct function
 *
 *   After installing both the timer and keyboard handler successfully,
 *   interrupts are enabled.
 *
 *   Requires that interrupts are disabled.
 *
 *   @param tickback Pointer to clock-tick callback function
 *   @return A negative error code on error, or 0 on success
 **/
int
handler_install(void (*tick)(unsigned int))
{
	/* Initialize and install timer handler */
	if (install_timer_handler(TIMER_IDT_ENTRY, call_timer_int_handler,
	                          tick) < 0) {
		return -1;
	}
	/* Initialize and install keyboard handler */
	if (install_keyboard_handler(KEY_IDT_ENTRY, call_keybd_int_handler) < 0) {
		return -1;
	}
	/* Interrupt handlers successfully installed, enable interrupts */
	enable_interrupts();
	return 0;
}

