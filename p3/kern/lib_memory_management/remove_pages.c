/** @file remove_pages.c
 *  @brief remove_pages syscall handler
 *
 *  @author Nicklaus Choo (nchoo)
 */

#include <logger.h>

int
remove_pages( void *base )
{
	log("base:%p", base);

	return 0;
}
