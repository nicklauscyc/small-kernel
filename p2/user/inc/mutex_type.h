/** @file mutex_type.h
 *  @brief This file defines the type for mutexes.
 */

#ifndef _MUTEX_TYPE_H
#define _MUTEX_TYPE_H

#include <stdint.h>

/** @brief Struct for a mutex
 *
 *  @param initialized 1 if mutex initialized, 0 otherwise
 *  @param serving Thread with this number will get the mutex.
 *  @param next_ticket The next ticket to be served.
 *  @param owner_tid Thread ID of thread that has acquired/locked the mutex
 */
typedef struct {
    int initialized;
    uint32_t serving;
	uint32_t next_ticket;
    int owner_tid;
} mutex_t;

#endif /* _MUTEX_TYPE_H */
