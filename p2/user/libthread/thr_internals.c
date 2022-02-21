/** @file thr_internals.c
 *	@brief Helper functions for thread library internals
 *	@author Nicklaus Choo (nchoo)
 */

#include <stdarg.h> /* va_list(), va_end() */
#include <simics.h> /* sim_puts() */
#include <syscall.h> /* gettid() */
#include <stdio.h> /* snprintf(), vsnprintf() */

#define LEN 256

/** @brief Prepends the thread id to printed format
 *
 *  Observe that va_end() is used after va_start() to prevent stack corruption.
 *
 *  @param format String format to print to
 *  @param ... All other arguments for string format
 */
void
tprintf(const char *format, ... )
{
	char str[LEN];

	/* Get tid and prepend to output*/
	int tid = gettid();
	int offset = snprintf(str, sizeof(str) - 1, "tid[%d]: ", tid);

	/* Print rest of output */
	va_list args;
	va_start(args, format);
	vsnprintf(str + offset, sizeof(str) - 1, format, args);
	sim_puts(str);
	va_end(args);

	return;
}

