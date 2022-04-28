/** @file iret_travel.h
 *  @brief Asm facilities for moving to user mode */

#ifndef _IRET_TRAVEL_H
#define _IRET_TRAVEL_H

#include <stdint.h> /* uint32_t */

/** @brief Sets processor state. This can be used for "travelling"
 *         to user mode. Only to be called from kernel mode.
 *
 *  Note: arguments are passed in the right order so that
 *  IRET may be called on them.
 *
 *  @param eip Handler address
 *  @param cs  User code segment
 *  @param eflags Eflags to adopt
 *  @param esp Stack pointer
 *  @param ss  Stack segment
 *
 *  @return Does not return. */
void iret_travel( uint32_t eip, uint32_t cs,
        uint32_t eflags, uint32_t esp, uint32_t ss );

#endif /* _IRET_TRAVEL_H */
