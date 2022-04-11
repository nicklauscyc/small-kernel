
#include <syscall.h>
#include <stdlib.h>
#include <stdio.h>
#include <simics.h>
#include "410_tests.h"

//TODO if I run this directly, the kernel will double fault
int main( int argc, char **argv )
{
	int i = 0;
	lprintf("argc:%d", argc);
	lprintf("argv:%p", argv);
	lprintf("argv[0]:%p", argv[0]);
	lprintf("argv[0]:%s", argv[0]);

	MAGIC_BREAK;

	while (argv[i]) {
		lprintf("argv[%d]:'%s'", i, argv[i]);
		++i;
	}
    lprintf("exec_args_test_helper done!");
	return 0;
}

