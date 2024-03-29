/** @file life_cycle.h
 *  @brief Life cycle functions made available without syscalls
 *  */

#ifndef LIFE_CYCLE_H_
#define LIFE_CYCLE_H_

int _fork( void );
int _exec( void );
int _vanish( void );

#endif /* LIFE_CYCLE_H_ */
