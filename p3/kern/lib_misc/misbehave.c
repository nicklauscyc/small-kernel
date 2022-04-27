
#include <logger.h> /* log_info() */
/** @brief Handler for readfile syscall. */
void
misbehave( int mode )
{
	log_info("UrMomsOS never misbehaves!");
	return;
}

