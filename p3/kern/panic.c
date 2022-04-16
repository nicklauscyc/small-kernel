/** @file panic.c
 *  @brief This file contains the panic() function which is called whenever
 *         a thread crashes.
 *
 *  We note that if "one thread in a multi-threaded application experiences
 *  a fatal exception, the application as a whole is unlikely to continue in a
 *  useful fashion." Because the crashed thread could have been holding on
 *  to locks such as mutexes and on crashing be unable to unlock those locks
 *  for other threads to use. Therefore whenever any child thread panics, the
 *  goal is to cause other threads to vanish as well, and so panic() invokes
 *  task_vanish().
 *
 *  @author edited by Nicklaus Choo (nchoo), acknowledgements below.
 *
 *  ---
 *
 *  Copyright (c) 1996-1995 The University of Utah and
 *  the Computer Systems Laboratory at the University of Utah (CSL).
 *  All rights reserved.
 *
 *  Permission to use, copy, modify and distribute this software is hereby
 *  granted provided that (1) source code retains these copyright, permission,
 *  and disclaimer notices, and (2) redistributions including binaries
 *  reproduce the notices in supporting documentation, and (3) all advertising
 *  materials mentioning features or use of this software display the following
 *  acknowledgement: ``This product includes software developed by the
 *  Computer Systems Laboratory at the University of Utah.''
 *
 *  THE UNIVERSITY OF UTAH AND CSL ALLOW FREE USE OF THIS SOFTWARE IN ITS "AS
 *  IS" CONDITION.  THE UNIVERSITY OF UTAH AND CSL DISCLAIM ANY LIABILITY OF
 *  ANY KIND FOR ANY DAMAGES WHATSOEVER RESULTING FROM THE USE OF THIS SOFTWARE.
 *
 *  CSL requests users of this software to return to csl-dist@cs.utah.edu any
 *  improvements that they make and grant CSL redistribution rights.
 */

#include <stdarg.h> /* va_list(), va_end() */
#include <logger.h> /* log_crit() */
#include <asm.h>	/* disable interrupts */

/** @brief This function is called by the assert() macro defined in assert.h;
 *         it's also a nice simple general-purpose panic function. Ceases
 *         execution of all running threads.
 *
 *  Any thread that calls panic should pass the reason for panic() in the
 *  argument fmt. Observe that va_end() is used after va_start() to prevent
 *  stack corruption.
 *
 *  @param fmt String to print out on panic
 *  @param ... Other arguments put into fmt
 *  @return Void.
 */
void panic( const char *fmt, ... )
{
	/* Print error that occurred */
	va_list args;
	va_start(args, fmt);
	vtprintf(fmt, args, CRITICAL_PRIORITY);
	va_end(args);

#include <simics.h>
	MAGIC_BREAK;

	disable_interrupts();
	while (1) {
		continue;
	}
    // call halt();

	return;
}
