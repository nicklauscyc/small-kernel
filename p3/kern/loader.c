/**
 * The 15-410 kernel project.
 * @name loader.c
 *
 * Functions for the loading
 * of user programs from binary
 * files should be written in
 * this file. The function
 * elf_load_helper() is provided
 * for your use.
 */
/*@{*/

/* --- Includes --- */
#include <string.h> /* memset() */
#include <stdio.h>
#include <malloc.h>
#include <exec2obj.h>
#include <loader.h>
#include <elf_410.h>
#include <assert.h> /* assert() */
#include <simics.h> /* lprintf() */
/* --- Local function prototypes --- */

///* Format of entries in the table of contents. */
//typedef struct {
//  const char execname[MAX_EXECNAME_LEN];
//  const char* execbytes;
//  int execlen;
//} exec2obj_userapp_TOC_entry;
//
///* The number of user executables in the table of contents. */
//extern const int exec2obj_userapp_count;
//
///* The table of contents. */
//extern const exec2obj_userapp_TOC_entry exec2obj_userapp_TOC[MAX_NUM_APP_ENTRIES];
//


/**
 * Copies data from a file into a buffer.
 *
 * @param filename   the name of the file to copy data from
 * @param offset     the location in the file to begin copying from
 * @param size       the number of bytes to be copied
 * @param buf        the buffer to copy the data into
 *
 * @return returns the number of bytes copied on succes; -1 on failure
 */
int getbytes( const char *filename, int offset, int size, char *buf )
{
	if (!filename) {
		return -1;
	}
	if (!buf) {
		return -1;
	}
	const char *current = filename;
	current += offset;
	int i = 0;
	while (i < size) {
		lprintf("i:%d", i);
		assert(*(current + i));
		*(buf + i) = *(current + i);
		i++;
	}
	/* Quick test for correct copying */
	for (int j = 0; j < size; j++) {
		assert(*(filename + offset + j) == *(buf + j));
	}
	return i;
}

int
copy2physical( simple_elf_t *se_hdr )
{
	/* copy text segment into physical memory */
	lprintf("e_entry:%lx, e_txtstart:%lx, e_datstart:%lx, e_rodatstart:%lx, "
	        "e_bssstart:%lx", se_hdr->e_entry, se_hdr->e_txtstart,
			se_hdr->e_datstart, se_hdr->e_rodatstart, se_hdr->e_bssstart);

	/* I feel like buf is physical memory. No buff is virtual memory */
	getbytes(se_hdr->e_fname,
	         (unsigned int) se_hdr->e_txtoff,
			 (unsigned int) se_hdr->e_txtlen,
	         (char *) se_hdr->e_txtstart);

	getbytes(se_hdr->e_fname,
	         (unsigned int) se_hdr->e_datoff,
			 (unsigned int) se_hdr->e_datlen,
	         (char *) se_hdr->e_datstart);

	getbytes(se_hdr->e_fname,
	         (unsigned int) se_hdr->e_rodatoff,
			 (unsigned int) se_hdr->e_rodatlen,
	         (char *) se_hdr->e_rodatstart);

	return 0;
}


/** @brief Given user program name such as './getpid', loads the program and
  *	       runs it
  */

int
execute_user_program( const char *fname )
{
	/* Load user program into helper struct */
	simple_elf_t se_hdr;
	memset(&se_hdr, 0, sizeof(simple_elf_t));
	int res = elf_load_helper(&se_hdr, fname);
	if(res == ELF_SUCCESS) lprintf("ELF_SUCCESS");
	if (res == ELF_NOTELF) lprintf("ELF_NOTELF");

	/* we try to use the physical addresses */
	copy2physical(&se_hdr);
	return 0;
}


/* we try to use the physical addresses */

/*@}*/
