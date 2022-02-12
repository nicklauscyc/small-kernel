

#include <syscall.h>
#include <stdlib.h> /* exit() */
int main() {
    char hello_message[] = "Starting tests\n";
    print(sizeof(hello_message), hello_message);

	exit(69);
}


