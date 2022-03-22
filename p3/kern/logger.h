/** @file logger.h
 *  @brief Functions that enable code tracing via logging
 *
 *  @author Nicklaus Choo (nchoo)
 */

#ifndef _LOGGER_H_
#define _LOGGER_H_

#define LOW_PRIORITY 1
#define MED_PRIORITY 2
#define HI_PRIORITY 3

/* Current logging level defined in kernel.c */
extern int log_level;

/* Function prototypes */
void log( const char *format, ... );
void log_med( const char *format, ...);
void log_hi( const char *format, ...);

#endif /* _LOGGER_H_ */


