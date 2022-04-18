/** @file logger.c
 *  @brief 3 levels of logging are implemented
 *
 *  if NDEBUG is set no logs will print
 *  else if NDEBUG is not set, each log will print iff log_level <= that
 *  logging function's priority. We follow Python's convention:
 *
 *  https://docs.python.org/3/howto/logging.html
 *
 *  To be more explicit:
 *  - log() will print whenever log_level <= DEBUG_PRIORITY
 *
 *    Toggle log_level to DEBUG_PRIORITY for debugging
 *    Typically used for diagnosing problems
 *
 *  - log_med() will print whenever log_level <= INFO_PRIORITY
 *
 *    Toggle log_level to INFO_PRIORITY to print info on expected behavior
 *    Confirmation that things are working as expected
 *
 *  - log_hi() will print whenever log_level <= WARN_PRIORITY
 *
 *    Toggle log_level to WARN_PRIORITY to print only warnings on behavior
 *    Indication that something potentially dangerous happened, indicative of
 *    some issue in the near future (e.g. 'low number of free physical frames').
 *    However the kernel is still working as expected.
 *
 */

#include <stdarg.h> /* va_list(), va_end() */
#include <stdio.h> /* snprintf(), vsnprintf() */
#include <simics.h> /* sim_puts() */
#include <scheduler.h> /* get_running_tid() */
#include <logger.h>
#include <assert.h> /* affirm_msg() */

#define LEN 256

/** @brief Prepends the thread id to printed format. Takes in a va_list as
 *         argument.
 *
 *  This function is called by logging functions and panic(), which must
 *  convert their variable number of arguments into a va_list before passing
 *  them to vtprintf().
 *
 *  Passing in an unrecognized priority will just lead to
 *  printing out an error message to let the user know that it has supplied
 *  the wrong arguments, no need to cause an assertion failure as the error
 *  is promptly displayed.
 *
 *  @param format String format to print to
 *  @param args A va_list of all arguments to include in format
 *  @param priority Logging priority.
 *  @return Void.
 */
void
vtprintf( const char *format, va_list args, int priority )
{
	char str[LEN];

	/* Get tid and prepend to output*/
	int tid = get_running_tid();
	int offset = 0;

	switch (priority) {

		case DEBUG_PRIORITY:
			offset = snprintf(str, sizeof(str) - 1, "tid[%d]: DEBUG: ", tid);
			break;

		case INFO_PRIORITY:
			offset = snprintf(str, sizeof(str) - 1, "tid[%d]: INFO: ", tid);
			break;

		case WARN_PRIORITY:
			offset = snprintf(str, sizeof(str) - 1, "tid[%d]: WARN: ", tid);
			break;

		case CRITICAL_PRIORITY:
			offset = snprintf(str, sizeof(str) - 1, "tid[%d]: CRITICAL: ", tid);
			break;

	    /* Improper use of vtprintf(), print error */
		default:
			snprintf(str, sizeof(str) - 1,
			         "tid[%d]: UNRECOGNIZED priority:%d for vtprintf()",
					 tid, priority);
			sim_puts(str);
			return;
	}
	/* Print rest of output, and a little extra for CRITICAL priority */
	offset += vsnprintf(str + offset, sizeof(str) - offset - 1, format, args);
	if (priority == CRITICAL_PRIORITY) {
		snprintf(str + offset, sizeof(str) - offset - 1,
		         "\nCrashing the kernel.");

		printf("%s", str);
	}

	sim_puts(str);
	//TODO print on actual hardware

	return;
}

/** @brief prints out log to console and kernel log as long as NDEBUG is not
 *         set. This log has priority DEBUG_PRIORITY
 *
 *  @param format String specifier to print
 *  @param ... Arguments to format string
 *  @return Void.
 */
void
log( const char *format, ... )
{
#ifndef NDEBUG
	if (log_level <= DEBUG_PRIORITY) {

		/* Construct va_list to pass on to vtprintf */
		va_list args;
		va_start(args, format);
		vtprintf(format, args, DEBUG_PRIORITY);
		va_end(args);
	}
#endif
	return;
}

/** @brief prints out log to console and kernel log as long as NDEBUG is not
 *         set. This log has priority INFO_PRIORITY
 *
 *  @param format String specifier to print
 *  @param ... Arguments to format string
 *  @return Void.
 */
void
log_info( const char *format, ... )
{
#ifndef NDEBUG
	if (log_level <= INFO_PRIORITY) {

		/* Construct va_list to pass on to vtprintf */
		va_list args;
		va_start(args, format);
		vtprintf(format, args, INFO_PRIORITY);
		va_end(args);
	}
#endif
	return;
}

/** @brief prints out log to console and kernel log as long as NDEBUG is not
 *         set. This log has priority WARN_PRIORITY
 *
 *  @param format String specifier to print
 *  @param ... Arguments to format string
 *  @return Void.
 */
void
log_warn( const char *format, ... )
{
#ifndef NDEBUG
	if (log_level <= WARN_PRIORITY) {
		/* Construct va_list to pass on to vtprintf */
		va_list args;
		va_start(args, format);
		vtprintf(format, args, WARN_PRIORITY);
		va_end(args);
	}
#endif
	return;
}

/** @brief prints out log to console and kernel log regardless of whether
 * 		   NDEBUG is set since this log has priority CRITICAL_PRIORITY
 * 		   which means that an unrecoverable failure in the kernel has occurred.
 *
 *  @pre Callers of log_crit must intend to crash the kernel
 *  @param format String specifier to print
 *  @param ... Arguments to format string
 *  @return Void.
 */
void
log_crit( const char *format, ... )
{
	if (log_level <= CRITICAL_PRIORITY) {

		/* Construct va_list to pass on to vtprintf */
		va_list args;
		va_start(args, format);
		vtprintf(format, args, CRITICAL_PRIORITY);
		va_end(args);
	}
}


