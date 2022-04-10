/** @file invalid_opcode_handler.c
 *  @brief Functions for handling invalid opcode faults
 */
#include <seg.h>	/* SEGSEL_KERNEL_CS */
#include <assert.h> /* panic() */

void
invalid_opcode_handler( int cs, int eip )
{
}
