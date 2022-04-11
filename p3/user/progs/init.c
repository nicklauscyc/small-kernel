/** @file init.c */

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

	/* For now just run shell, in the future fork and wait on shell. */
	char shell[] = "shell";
	char *args[] = {"shell", 0};
	exec(shell, args);
}

