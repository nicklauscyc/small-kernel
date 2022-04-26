
#include <stdint.h> /* uint32_t */
#include <logger.h>

int
thread_fork( uint32_t ebx, uint32_t ecx, uint32_t edx, uint32_t edi,
	uint32_t esi )
{
	log_warn("hello from thread_fork!");
	return 0;
}

