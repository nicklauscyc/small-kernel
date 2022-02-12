

#include <syscall.h>
#include <stdlib.h> /* exit() */
#include <simics.h> /* lprintf */

int main() {
	exit(69);
}

/* Should run cat and return error and different exit status */
void test_exec() {
    char *argvec[] = { "cat", ".", NULL };
    int err = exec(argvec[0], argvec);
}

void test_fork() {
    char hello_dad[] = "Hello from parent\n";
    char hello_son[] = "Hello from child\n";


    if (fork()) {
        print(sizeof(hello_dad), hello_dad);
    } else {
        print(sizeof(hello_son), hello_son);
    }

}
