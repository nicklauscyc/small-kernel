/** @file asm_fault_handlers.h
 *  @brief Fault handlers */

#ifndef ASM_FAULT_HANDLERS_H_
#define ASM_FAULT_HANDLERS_H_

void call_divide_handler( void );
void call_debug_handler( void );
void call_breakpoint_handler( void );
void call_overflow_handler( void );
void call_bound_handler( void );
void call_invalid_opcode_handler( void );
void call_float_handler( void );
void call_segment_not_present_handler( void );
void call_stack_fault_handler( void );
void call_general_protection_handler( void );
void call_alignment_check_handler( void );
void call_non_maskable_handler( void );
void call_machine_check_handler( void );
void call_simd_handler( void );

#endif /* ASM_FAULT_HANDLERS_H_ */
