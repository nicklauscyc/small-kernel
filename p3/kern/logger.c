/** @file logger.c
 *  @brief 3 levels of logging are implemented
 *
 *  if NDEBUG is set no logs will print
 *  else if NDEBUG is not set, each log will print iff log_level <= that
 *  logging function's priority. To be more explicit:
 *
 *  - log() will print whenever log_level <= LOW_PRIOIRTY
 *  - log_med() will print whenever log_level <= MED_PRIOIRITY
 *  - log_hi() will print whenever log_level <= HI_PRIORITY
 *
 */

#include <stdarg.h> /* va_list(), va_end() */
#include <stdio.h> /* snprintf(), vsnprintf() */
#include <simics.h> /* sim_puts() */
#include <scheduler.h> /* get_running_tid() */
#include <logger.h>

#define LEN 256

/** @brief Prepends the thread id to printed format. Takes in a va_list as
 *         argument.
 *
 *  This function is called by logging functions and panic(), which must
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
	int tid = get_running_tid();
	int offset = snprintf(str, sizeof(str) - 1, "tid[%d]: ", tid);

	/* Print rest of output */
	vsnprintf(str + offset, sizeof(str) - offset - 1, format, args);
	sim_puts(str);

	return;
}

/** @brief prints out log to console and kernel log as long as NDEBUG is not
 *         set. This log has priority LOW_PRIORITY
 *
 *  @param format String specifier to print
 *  @param ... Arguments to format string
 *  @return Void.
 */
void
log( const char *format, ... )
{
#ifndef NDEBUG
	if (log_level <= LOW_PRIORITY) {
		/* Construct va_list to pass on to vtprintf */
		va_list args;

		va_start(args, format);
		vtprintf(format, args);
		va_end(args);
	}
#endif
	return;
}

/** @brief prints out log to console and kernel log as long as NDEBUG is not
 *         set. This log has priority MED_PRIORITY
 *
 *  @param format String specifier to print
 *  @param ... Arguments to format string
 *  @return Void.
 */
void
log_med( const char *format, ... )
{
#ifndef NDEBUG
	if (log_level <= MED_PRIORITY) {
		/* Construct va_list to pass on to vtprintf */
		va_list args;

		va_start(args, format);
		vtprintf(format, args);
		va_end(args);
	}
#endif
	return;
}

/** @brief prints out log to console and kernel log as long as NDEBUG is not
 *         set. This log has priority MED_PRIORITY
 *
 *  @param format String specifier to print
 *  @param ... Arguments to format string
 *  @return Void.
 */
void
log_hi( const char *format, ... )
{
#ifndef NDEBUG
	if (log_level <= HI_PRIORITY) {
		/* Construct va_list to pass on to vtprintf */
		va_list args;

		va_start(args, format);
		vtprintf(format, args);
		va_end(args);
	}
#endif
	return;
}
