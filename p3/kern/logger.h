/** @file logger.h
 *  @brief Functions that enable code tracing via logging
 *
 *  @author Nicklaus Choo (nchoo)
 */

#include <stdarg.h> /* va_list() */

#ifndef LOGGER_H_
#define LOGGER_H_

#define DEBUG_PRIORITY 1
#define INFO_PRIORITY 2
#define WARN_PRIORITY 3
#define CRITICAL_PRIORITY 4

/* Current logging level defined in kernel.c */
extern int log_level;

/* Function prototypes */
void vtprintf( const char *format, va_list args, int priority );
void log( const char *format, ... );
void log_info( const char *format, ...);
void log_warn( const char *format, ...);
void log_crit( const char *format, ...);

#endif /* LOGGER_H_ */

