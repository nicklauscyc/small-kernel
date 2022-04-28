/** @file readfile.c
 *  @brief Readfile syscall handler
 */
#include <asm.h>				/* outb */
#include <assert.h>				/* affirm */
#include <console.h>			/* putbytes */
#include <exec2obj.h>			/* MAX_EXECNAME_LEN */
#include <memory_manager.h>		/* is_valid_user_pointer/string */
#include <interrupt_defines.h>	/* INT_CTL_PORT, INT_ACK_CURRENT */

/** @brief Handler for readfile syscall.
 *
 *  @param filename name of file to read from
 *  @param buf Buffer in which to place file's bytes
 *  @param count Number of bytes to read
 *  @param offset Offset into file to start reading from
 *  @return Number of bytes read on success, negative value on failure */
int
readfile( char *filename, char *buf, int count, int offset )
{
	/* Acknowledge interrupt */
    outb(INT_CTL_PORT, INT_ACK_CURRENT);

	if (!is_valid_user_string(filename, MAX_EXECNAME_LEN))
		return -1;

	/* Ensure all of buf is valid */
	for	(int i=0; i < count; ++i) {
		if (!is_valid_user_pointer(buf + i, READ_WRITE))
			return -1;
	}

	return getbytes(filename, offset, count, buf);
}
