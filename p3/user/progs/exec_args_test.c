
#include <syscall.h>
#include <stdlib.h>
#include <stdio.h>
#include <simics.h>
#include "410_tests.h"

int main(){

	char *name = "exec_args_test_helper\0";
	char *args[] = {name, "hello\0", "morty123\0", 0};
	exec(name, args);
	exit(-1);
}

