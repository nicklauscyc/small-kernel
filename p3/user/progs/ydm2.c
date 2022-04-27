#include <syscall.h>
#include <stdlib.h>
#include <stdio.h>
#include "410_tests.h"
#include <report.h>

DEF_TEST_NAME("test_all:");



int
main()
{
	int pid, pid2;
	int res, res2;
	char test[] = "yield_desc_mkrun";
	char *args[] = {"yield_desc_mkrun", 0};

	report_start(START_CMPLT);


	pid = fork();
	if (!pid) {
		report_misc("After fork(): I am a child!");
		pid2 = fork();
		if (!pid2) {
		exec(test, args);
		} else {
			pid2 = fork();

			res2 = wait(0);
			lprintf("res2:%d", res2);
		}
	} else {
		res = wait(0);
		lprintf("res:%d", res);
	}

	report_end(END_SUCCESS);
	exit(0);
}
