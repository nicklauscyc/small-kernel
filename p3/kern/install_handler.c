/** @file install_handler.c
 *  @brief Contains functions that install the timer and keyboard handlers
 *
 *  @author Nicklaus Choo (nchoo)
 *  @bug No known bugs.
 */

#include <install_handler.h>
#include <asm.h>			/* idt_base() */
#include <idt.h>			/* IDT_PF */
#include <seg.h>            /* SEGSEL_KERNEL_CS */
#include <assert.h>			/* assert() */
#include <stddef.h>         /* NULL */
#include <keyhelp.h>		/* KEY_IDT_ENTRY */
#include <timer_driver.h>   /* init_timer() */
#include <keybd_driver.h>   /* init_keybd() */
#include <timer_defines.h>  /* TIMER_IDT_ENTRY */
#include <lib_console/readline.h> /* init_readline() */
#include <interrupt_defines.h>
#include <lib_life_cycle/life_cycle.h>
#include <asm_interrupt_handler.h>			/* call_timer_int_handler(),
												call_keybd_int_handler() */
#include <asm_misc_handlers.h>
#include <asm_fault_handlers.h>
#include <asm_console_handlers.h>
#include <asm_life_cycle_handlers.h>
#include <asm_thread_management_handlers.h>
#include <asm_memory_management_handlers.h>
#include <tests.h>                          /* install_test_handler() */

#include <syscall_int.h> /* *_INT */


#define TEST_INT SYSCALL_RESERVED_0

/*********************************************************************/
/*                                                                   */
/* Internal helper functions                                         */
/*                                                                   */
/*********************************************************************/

/** @brief Installs an interrupt handler at idt_entry
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
install_handler_in_idt(int idt_entry, asm_wrapper_t *asm_wrapper, int dpl,
                       int gate_type)
{
  if (!asm_wrapper) {
	  return -1;
  }
  /* Get gate address */
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
  data_upper = offset_upper | PRESENT | dpl | gate_type;

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
	return install_handler_in_idt(idt_entry, asm_wrapper, DPL_0, D32_TRAP);
}

/** @brief General function to install a handler without an init function
 *
 *  @param idt_entry Index in IDT to install
 *  @param init Initialization function if needed for handler installation
 *  @param asm_wrapper Assembly wrapper to call the handler
 *  @param dpl DPL
 *  @param gate_type Wether it is a trap gate or interrupt gate
 *  @return 0 on success, -1 on error
 */
int
install_handler( int idt_entry, init_func_t *init, asm_wrapper_t *asm_wrapper,
                 int dpl, int gate_type )
{
	if (!asm_wrapper) {
		return -1;
	}
	if ((gate_type != D32_TRAP) && (gate_type != D32_INTERRUPT)) {
		return -1;
	}
	if (init) {
		init();
	}
	return install_handler_in_idt(idt_entry, asm_wrapper, dpl, gate_type);
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

	/* The keyboard handler is made an interrupt gate. This is because if we
	 * make it a trap gate instead, the timer interrupt could trigger a context
	 * switch before we acknowledge the keybd interrupt signal, thereby
	 * invalidating all user input until we context switch back to the thread
	 * handling the keyboard interrupt. */
	return install_handler_in_idt(idt_entry, asm_wrapper, DPL_0, D32_INTERRUPT);
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
    /* Install test syscall for tests spanning user and kernel mode. */
    if (install_test_handler(TEST_INT, call_test_int_handler) < 0) {
        return -1;
    }
	if (install_timer_handler(TIMER_IDT_ENTRY, call_timer_int_handler,
	                          tick) < 0) {
		return -1;
	}
	if (install_keyboard_handler(KEY_IDT_ENTRY, call_keybd_int_handler) < 0) {
		return -1;
	}

	/* Lib thread management */
	if (install_handler(GETTID_INT, NULL, call_gettid, DPL_3, D32_TRAP) < 0) {
		return -1;
	}

	if (install_handler(GET_TICKS_INT, NULL, call_get_ticks, DPL_3,
		D32_TRAP) < 0) {
		return -1;
	}

	if (install_handler(YIELD_INT, NULL, call_yield, DPL_3, D32_TRAP) < 0) {
		return -1;
	}

	if (install_handler(DESCHEDULE_INT, NULL, call_deschedule, DPL_3,
		D32_TRAP) < 0) {
		return -1;
	}

	if (install_handler(MAKE_RUNNABLE_INT, NULL, call_make_runnable, DPL_3,
		D32_TRAP) < 0) {
		return -1;
	}

	if (install_handler(SWEXN_INT, NULL, call_swexn, DPL_3,
		D32_TRAP) < 0) {
		return -1;
	}

	if (install_handler(SLEEP_INT, NULL, call_sleep, DPL_3,
		D32_TRAP) < 0) {
		return -1;
	}

	/* Lib lifecycle*/
	//TODO are these DPL_3 or DPL_0
	if (install_handler(FORK_INT, NULL, call_fork, DPL_3, D32_TRAP) < 0) {
		return -1;
	}
	if (install_handler(EXEC_INT, NULL, call_exec, DPL_3, D32_TRAP) < 0) {
		return -1;
	}
	init_vanish();
	if (install_handler(VANISH_INT, NULL, call_vanish, DPL_3, D32_TRAP) < 0) {
		return -1;
	}
	if (install_handler(TASK_VANISH_INT, NULL, call_task_vanish, DPL_3,
		D32_TRAP) < 0) {
		return -1;
	}
	if (install_handler(SET_STATUS_INT, NULL, call_set_status, DPL_3,
		D32_TRAP) < 0) {
		return -1;
	}
	if (install_handler(WAIT_INT, NULL, call_wait, DPL_3, D32_TRAP) < 0) {
		return -1;
	}
	if (install_handler(THREAD_FORK_INT, NULL, call_thread_fork, DPL_3,
		D32_TRAP) < 0) {
		return -1;
	}

	/* Lib memory management */
	if (install_handler(NEW_PAGES_INT, NULL, call_new_pages, DPL_3,
		D32_TRAP) < 0) {
		return -1;
	}
	if (install_handler(REMOVE_PAGES_INT, NULL, call_remove_pages, DPL_3,
		D32_TRAP) < 0) {
		return -1;
	}
	if (install_handler(IDT_PF, NULL, call_pagefault_handler, DPL_3,
		D32_TRAP) < 0) {
		return -1;
	}

	/* Lib console */
	if (install_handler(READLINE_INT, init_readline,
		call_readline, DPL_3, D32_TRAP) < 0) {
		return -1;
	}
	if (install_handler(PRINT_INT, NULL, call_print, DPL_3, D32_TRAP) < 0) {
		return -1;
	}
	if (install_handler(GET_CURSOR_POS_INT, NULL, call_get_cursor_pos, DPL_3,
		D32_TRAP) < 0) {
		return -1;
	}
	if (install_handler(SET_CURSOR_POS_INT, NULL, call_set_cursor_pos, DPL_3,
		D32_TRAP) < 0) {
		return -1;
	}
	if (install_handler(SET_TERM_COLOR_INT, NULL, call_set_term_color_handler,
		DPL_3, D32_TRAP) < 0) {
		return -1;
	}

	/* Lib misc */
	if (install_handler(READFILE_INT, NULL, call_readfile, DPL_3,
		D32_TRAP) < 0) {
		return -1;
	}

	if (install_handler(HALT_INT, NULL, call_halt, DPL_3, D32_TRAP) < 0) {
		return -1;
	}
	if (install_handler(MISBEHAVE_INT, NULL, call_misbehave, DPL_3,
		D32_TRAP) < 0) {
		return -1;
	}

	/* Fault handlers */
	if (install_handler(IDT_DE, NULL, call_divide_handler, DPL_3,
		D32_TRAP) < 0) {
		return -1;
	}

	if (install_handler(IDT_DB, NULL, call_debug_handler, DPL_3,
		D32_TRAP) < 0) {
		return -1;
	}

	if (install_handler(IDT_BP, NULL, call_breakpoint_handler, DPL_3,
		D32_TRAP) < 0) {
		return -1;
	}

	if (install_handler(IDT_OF, NULL, call_overflow_handler, DPL_3,
		D32_TRAP) < 0) {
		return -1;
	}

	if (install_handler(IDT_BR, NULL, call_bound_handler, DPL_3,
		D32_TRAP) < 0) {
		return -1;
	}

	if (install_handler(IDT_UD, NULL, call_invalid_opcode_handler, DPL_3,
		D32_TRAP) < 0) {
		return -1;
	}

	if (install_handler(IDT_NM, NULL, call_float_handler, DPL_3,
		D32_TRAP) < 0) {
		return -1;
	}

	if (install_handler(IDT_NP, NULL, call_segment_not_present_handler, DPL_3,
		D32_TRAP) < 0) {
		return -1;
	}

	if (install_handler(IDT_SS, NULL, call_stack_fault_handler, DPL_3,
		D32_TRAP) < 0) {
		return -1;
	}

	if (install_handler(IDT_GP, NULL, call_general_protection_handler, DPL_3,
		D32_TRAP) < 0) {
		return -1;
	}

	if (install_handler(IDT_AC, NULL, call_alignment_check_handler, DPL_3,
		D32_TRAP) < 0) {
		return -1;
	}

	if (install_handler(IDT_XF, NULL, call_simd_handler, DPL_3,
		D32_TRAP) < 0) {
		return -1;
	}

	/* Install irrecoverable exceptions as interrupt so we can cleanly print
	 * an error and exit */
	if (install_handler(IDT_NMI, NULL, call_non_maskable_handler, DPL_3,
		D32_INTERRUPT) < 0) {
		return -1;
	}

	if (install_handler(IDT_MC, NULL, call_machine_check_handler, DPL_3,
		D32_INTERRUPT) < 0) {
		return -1;
	}
	return 0;
}

