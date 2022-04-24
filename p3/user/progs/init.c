/** @file init.c */

#include <simics.h>
#include <syscall.h>
#include <stdio.h>

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
	// Copied from init.c
    int exitstatus;
    char shell[] = "shell";
    char * args[] = {shell, 0};

    while(1) {
        pid = fork();
        if (!pid)
            exec(shell, args);

		int child_tid;
		int total_cleanup = 0;
        do {
			child_tid = wait(&exitstatus);
			if (child_tid >= 0) {
				total_cleanup++;
			}
		} while (child_tid != pid);

        printf("Shell exited with status %d; starting it back up...", exitstatus);
    }
	return 0;
}

