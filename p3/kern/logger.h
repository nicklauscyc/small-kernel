/** @file logger.h
 *  @brief Functions that enable code tracing via logging
 *
 *  @author Nicklaus Choo (nchoo)
 */

#ifndef _LOGGER_H_
#define _LOGGER_H_

#define DEBUG_PRIORITY 1
#define INFO_PRIORITY 2
#define WARN_PRIORITY 3

/* Current logging level defined in kernel.c */
extern int log_level;

/* Function prototypes */
void log( const char *format, ... );
void log_info( const char *format, ...);
void log_warn( const char *format, ...);

#endif /* _LOGGER_H_ */


