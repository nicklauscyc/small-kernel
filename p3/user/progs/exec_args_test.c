
#include <syscall.h>
#include <stdlib.h>
#include <stdio.h>
#include <simics.h>
#include "410_tests.h"
#include "test.h" /* run_test */
#define PD_CONSISTENCY  3

int
pd_test( void )
{
    return run_test(PD_CONSISTENCY);
}

int main(){
    // check pd consistency
    pd_test();

	char *name = "exec_args_test_helper\0";
    // check pd consistency
    pd_test();

//	char *args[] = {name, "hello\0", "morty123\0", 0};
	char *args[] = {name, 0};
    // check pd consistency
    pd_test(); // all these pd_test()'s pass


	exec(name, args);

	exit(-1);
}

