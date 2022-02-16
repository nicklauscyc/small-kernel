/** @file mutex_type.h
 *  @brief This file defines the type for mutexes.
 */

#ifndef _MUTEX_TYPE_H
#define _MUTEX_TYPE_H

#include <stdint.h>

typedef struct {
    int initialized;
    uint32_t serving;      /* Ticket being served */
    uint32_t next_ticket;  /* Next ticket to be picked. */
    int owner_tid;
} mutex_t;

#endif /* _MUTEX_TYPE_H */
