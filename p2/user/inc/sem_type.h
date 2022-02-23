/** @file sem_type.h
 *  @brief This file defines the type for semaphores.
 */

#ifndef _SEM_TYPE_H
#define _SEM_TYPE_H

#include <mutex.h>
#include <cond.h>

/** @brief Struct for semaphore
 *
 *  WARNING: DO NOT EVER INTERACT WITH THE MUTEX AND CONDITION VARIABLE
 *  OF THE SEMAPHORE DIRECTLY. STICK TO THE INTERFACE PROVIDED FOR THE
 *  SEMAPHORE FAMILY OF FUNCTIONS OR RISK UNDEFINED BEHAVIOR.
 *
 *  @param count Current count of semaphore
 *  @param initialized 1 if initialized, 0 otherwise
 *  @param mux Mutex for the semaphore
 *  @param cv Condition variable for the semaphore
 */
typedef struct {
  int count;
  int initialized;
  mutex_t mux;
  cond_t cv;
} sem_t;

#endif /* _SEM_TYPE_H */


