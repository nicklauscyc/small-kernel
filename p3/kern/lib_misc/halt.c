/** @file halt.c
 *  @brief Halt syscall handler
 */
#include <assert.h>				/* panic */
#include <simics.h>				/* sim_halt */

/** @brief Handler for readfile syscall. */
void
halt( void )
{
	sim_halt();

	panic("HALT UNIMPLEMENTED FOR REAL HARDWARE! "
			"Please do not contact developers.");
}
