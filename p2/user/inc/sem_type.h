/** @file sem_type.h
 *  @brief This file defines the type for semaphores.
 */

#ifndef _SEM_TYPE_H
#define _SEM_TYPE_H

#include <mutex.h>
#include <cond.h>

typedef struct sem
{
  /* fill this in */
  int total;
  int count;
  int initialized;
  mutex_t *muxp;
  cond_t *cvp;
  mutex_t mux;
  cond_t cv;

} sem_t;

#endif /* _SEM_TYPE_H */


