
#include "simics.h"
#include <assert.h>
#include "syscall.h"
#include <stdlib.h>

int
main()
{
	int pid = fork();

	/* child */
	if (pid == 0) {
		sleep(10000);
	} else {
		affirm(wait(0) < 0);
		int status;
		affirm(wait(&status) == pid);
		lprintf("bad_status_ptr: END_SUCCESS");
	}
	exit(69);
}


