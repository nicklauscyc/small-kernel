/** @file vanish.c
 *  @brief Implements vanish()
 */
#include <x86/asm.h>   /* outb() */
#include <x86/interrupt_defines.h> /* INT_CTL_PORT, INT_ACK_CURRENT */
#include <logger.h>    /* log() */
#include <timer_driver.h>		/* get_total_ticks() */
#include <lib_thread_management/thread_management.h> /* _deschedule() */
#include <lib_thread_management/mutex.h> /* _deschedule() */

static mutex_t hacky_vanish_mux;
static int hacky_vanish_mux_init = 0;

void
_vanish( void )
{
	log_info("call _vanish()");

	if (!hacky_vanish_mux_init) {
		mutex_init(&hacky_vanish_mux);
		hacky_vanish_mux_init = 1;
		log_warn("&hacky_vanis_mux:%p", &hacky_vanish_mux);
	}
	// Let's try locking to 'deschedule' the vanished thread
	mutex_lock(&hacky_vanish_mux);

	// FIXME deschedule does not seem to work, bug?
	//int reject = 0;
	//_deschedule(&reject);

	// FIXME when we loop forever we observe that some threads that have
	// yet to vanish never run?
	//while(1)
	//{
	//	continue;
	//}

}


void
vanish( void )
{
	/* Acknowledge interrupt immediately */
	outb(INT_CTL_PORT, INT_ACK_CURRENT);
	log_info("call vanish");
	_vanish();
}



