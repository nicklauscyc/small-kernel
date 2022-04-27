/** @file halt.c
 *  @brief Halt syscall handler
 */
#include <assert.h>		/* panic */
#include <simics.h>		/* sim_halt */
#include "call_halt.h"	/* call_hlt */

/** @brief Halt OS. Also used as handler for halt syscall. */
void
halt( void )
{
	/* HLT is no-op in simics, therefore call simics halt */
	sim_halt();

	call_hlt();

	/* NOTREACHED */
	panic("NOTREACHED: After halt instruction");
}
