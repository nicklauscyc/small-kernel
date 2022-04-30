/** @file misbehave.c
 *
 *  Very useful definitions only! */

#include <logger.h> /* log_info() */

/** @brief Handler for misbehave syscall. */
void
misbehave( int mode )
{
	log_info("UrMomsOS never misbehaves!");
	return;
}

