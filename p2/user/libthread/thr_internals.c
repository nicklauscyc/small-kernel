/** @file thr_internals.c
 *	@brief Helper functions for thread library internals
 *	@author Nicklaus Choo (nchoo)
 */

#include <stdarg.h> /* va_list(), va_end() */
#include <simics.h> /* sim_puts() */
#include <syscall.h> /* gettid() */
#include <stdio.h> /* snprintf(), vsnprintf() */

#define LEN 256

/** @brief Prepends the thread id to printed format. Takes in a va_list as
 *         argument.
 *
 *  This function is only called by panic() and tprintf(), both of which must
 *  convert their variable number of arguments into a va_list before passing
 *  them to vtprintf().
 *
 *  @param format String format to print to
 *  @param args A va_list of all arguments to include in format
 *  @return Void.
 */
void
vtprintf( const char *format, va_list args )
{
	char str[LEN];

	/* Get tid and prepend to output*/
	int tid = gettid();
	int offset = snprintf(str, sizeof(str) - 1, "tid[%d]: ", tid);

	/* Print rest of output */
	vsnprintf(str + offset, sizeof(str) - 1, format, args);
	sim_puts(str);

	return;
}

/** @brief Wrapper called directly in source code for printing statements
 *         with thread ID prepended
 *
 *  We need this wrapper since if one tries to pass a va_list into another
 *  variadic function, the va_list will get butchered. Observe that va_end()
 *  is used after va_start() to prevent stack corruption.
 *
 *  @param format String format for printing
 *  @param ... Variable number of arguments
 *  @return Void.
 */
void
tprintf( const char *format, ... )
{
	/* Construct va_list to pass on to vtprintf */
	va_list args;

	va_start(args, format);
	vtprintf(format, args);
	va_end(args);

	return;
}
