/** @file autostack_internals.h
 *  @brief Function prototypes for page fault exception handlers
 *	@author Nicklaus Choo (nchoo)
 */

void Swexn( void *esp3, swexn_handler_t eip, void *arg, ureg_t *newureg );
void pf_swexn_handler( void *arg, ureg_t *ureg );
void child_pf_handler( void *arg, ureg_t *ureg );
void install_child_pf_handler( void *esp3 );

