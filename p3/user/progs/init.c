/** @file init.c */

#include <simics.h>
#include <syscall.h>

// TODO: Fork and run idle.
//		 Then fork and run shell + wait (for now what can we do instead of actually waiting?)

int
main()
{
	int pid = fork();

	if (!pid) {
		char idle[] = "idle";
		char *args[] = {"idle", 0};
		exec(idle, args);
	}

//	int pid2 = fork();
//
//	if (!pid2) {
		/* For now just run shell, in the future fork and wait on shell. */

		char shell[] = "shell";
		char *args[] = {"shell", 0};
		exec(shell, args);

//	}
//
//	int status;
//	while (pid2 != wait(&status));
//
//	lprintf("Shell exited with status %d", status);

	return 0;
}

