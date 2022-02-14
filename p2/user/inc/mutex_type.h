/** @file mutex_type.h
 *  @brief This file defines the type for mutexes.
 */

#ifndef _MUTEX_TYPE_H
#define _MUTEX_TYPE_H

typedef struct {
    int initialized;
    int serving;      /* Ticket being served */
    int next_ticket;  /* Next ticket to be picked. */
    int owner_tid;
} mutex_t;

#endif /* _MUTEX_TYPE_H */
