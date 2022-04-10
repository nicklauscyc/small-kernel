/** @file divide_handler.c
 *  @brief Functions for handling division faults
 */
#include <assert.h> /* panic() */
#include <install_handler.h> /* install_handler_in_idt() */

/** @brief Prints out the offending address on and calls panic()
 *
 *  @return Void.
 */
void
divide_handler( void )
{
	// TODO
}
