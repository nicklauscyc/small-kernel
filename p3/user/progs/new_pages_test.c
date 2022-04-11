
#include <syscall.h>
#include <stdlib.h>
#include <stdio.h>
#include <simics.h>
#include "410_tests.h"
#include <assert.h>

int main(){

	char *name = (char *) (0x1000000 + 7*PAGE_SIZE);
	int len = 2 * PAGE_SIZE;
	int res = new_pages(name, len);
	assert(res == 0);
	assert(new_pages(name, len) < 0);
	lprintf("new_pages allocated");
	/* Test writing to name */
	lprintf("name:%p", name);
	*name = 'a';
	lprintf("new_pages test passed");
	exit(0);
}

