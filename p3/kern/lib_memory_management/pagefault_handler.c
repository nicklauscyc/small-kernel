/** @file pagefault_handler.c
 *  @brief Functions for page fault handling
 *  TODO write a macro for syscall handler/install handler wrappers
 *  @author Nicklaus Choo (nchoo)
 */
#include <assert.h> /* panic() */
#include <install_handler.h> /* install_handler_in_idt() */
#include <x86/cr.h> /* get_cr2() */

#define PRESENT_BIT			(1 << 0)
#define READ_WRITE_BIT		(1 << 1)
#define USER_SUPERVISOR_BIT (1 << 2)
#define RESERVED_BIT_BIT	(1 << 3)

/** @brief Prints out the offending address on and calls panic()
 *
 *  @return Void.
 */
void
pagefault_handler( int error_code )
{
	/* TODO: acknowledge signal and call user handler  */

	uint32_t faulting_vm_address = get_cr2();

	char user_mode[] = "[USER-MODE]";
	char supervisor_mode[] = "[SUPERVISOR-MODE]";

	char *mode = (error_code & USER_SUPERVISOR_BIT) ?
		user_mode : supervisor_mode;

	if (!(error_code & PRESENT_BIT))
		panic("%s Page fault at vm address:0x%lx! Page not present.",
				mode, faulting_vm_address);

	if (error_code & READ_WRITE_BIT)
		panic("%s Page fault at vm address:0x%lx! Writing into read-only page",
				mode, faulting_vm_address);

	if (error_code & RESERVED_BIT_BIT)
		panic("%s Page fault at vm address:0x%lx! Writing into reserved bits",
				mode, faulting_vm_address);
}
