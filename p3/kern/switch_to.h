#ifndef SWITCH_TO_H_
#define SWITCH_TO_H_

#include <stdint.h> /* uint32_t */

void switch_to( uint32_t *esp, uint32_t **old_esp, void *ptd );

#endif /* SWITCH_TO_H_ */
