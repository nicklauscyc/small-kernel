/** @brief wait.c
 *
 *  Implements wait()
 */
#include <x86/asm.h>   /* outb() */
#include <x86/interrupt_defines.h> /* INT_CTL_PORT, INT_ACK_CURRENT */
#include <logger.h>    /* log() */
#include <timer_driver.h>		/* get_total_ticks() */
#include <scheduler.h>			/* yield_execution() */
#include <task_manager_internal.h>
#include <lib_thread_management/hashmap.h>
#include <memory_manager.h> /* get_initial_pd() */
#include <x86/cr.h>		/* {get,set}_{cr0,cr3} */
#include <malloc.h> /* sfree() */

