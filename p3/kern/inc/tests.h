#ifndef TESTS_H_
#define TESTS_H_

#include <stdint.h> /* uint32_t */

int test_int_handler( int test_num );
int install_test_handler( uint32_t idt_entry, asm_wrapper_t *asm_wrapper );
void call_test_int_handler( void );

#endif /* TESTS_H_ */
