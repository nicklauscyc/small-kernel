/** @file vanish.c
 *  @brief Implements vanish()
 */
#include <x86/asm.h>   /* outb() */
#include <x86/interrupt_defines.h> /* INT_CTL_PORT, INT_ACK_CURRENT */
#include <logger.h>    /* log() */
#include <timer_driver.h>		/* get_total_ticks() */
#include <scheduler.h>			/* yield_execution() */
#include <task_manager_internal.h>

//static mutex_t hacky_vanish_mux;
//static int hacky_vanish_mux_init = 0;


void
_vanish( void ) // int on_error )
{
	tcb_t *tcb = get_running_thread();
	log_warn("Vanishing");


	// TODO error checking later
	/* Check if vanished because of an error or no? */
	//if (on_error) {
	//	if (

	//}



	/* Add to */
	pcb_t *owning_task = tcb->owning_task;
	mutex_lock(&(owning_task->set_status_vanish_wait_mux));

	/* Clean up resources */



	/*After we run this we will never be run ever again */
	/* TODO so when should cleanup happen? */
	affirm(yield_execution(DEAD, -1, NULL, NULL) == 0);

}


void
vanish( void )
{
	/* Acknowledge interrupt immediately */
	outb(INT_CTL_PORT, INT_ACK_CURRENT);
	log_info("call vanish");
	_vanish();
}



