
#include "simics.h"
#include <assert.h>
#include "syscall.h"
#include <stdlib.h>
#include <stdio.h>
#include "variable_queue.h"

#define LIST_LEN 10

typedef struct node
{
	Q_NEW_LINK(node) link;
	int data;
} node_t;

Q_NEW_HEAD(list_t, node);

void test_append( void )
{
	list_t list1;
	list_t list2;

	Q_INIT_HEAD(&list1);
	Q_INIT_HEAD(&list2);

	node_t node[LIST_LEN];

	Q_INIT_ELEM(&node[0], link);
	Q_INIT_ELEM(&node[1], link);
	Q_INIT_ELEM(&node[2], link);
	Q_INIT_ELEM(&node[3], link);
	Q_INIT_ELEM(&node[4], link);
	Q_INIT_ELEM(&node[5], link);
	Q_INIT_ELEM(&node[6], link);
	Q_INIT_ELEM(&node[7], link);
	Q_INIT_ELEM(&node[8], link);
	Q_INIT_ELEM(&node[9], link);
	Q_INIT_ELEM(&node[10], link);

	assert(Q_GET_FRONT(&list1) == NULL);
	assert(Q_GET_TAIL(&list1) == NULL);
	assert(Q_GET_FRONT(&list2) == NULL);
	assert(Q_GET_TAIL(&list2) == NULL);

	/* Both empty */
	Q_APPEND(&list1, &list2, link);
	assert(Q_GET_FRONT(&list1) == NULL);
	assert(Q_GET_TAIL(&list1) == NULL);
	assert(Q_GET_FRONT(&list2) == NULL);
	assert(Q_GET_TAIL(&list2) == NULL);


	/* Suffix empty */
	Q_INSERT_TAIL(&list2, &node[0], link);
	assert(Q_GET_FRONT(&list2) == &node[0]);
	assert(Q_GET_TAIL(&list2) == &node[0]);
	assert(!Q_GET_NEXT(&node[0], link));
	assert(!Q_GET_PREV(&node[0], link));


	Q_APPEND(&list1, &list2, link);
	assert(Q_GET_FRONT(&list2) == NULL);
	assert(Q_GET_TAIL(&list2) == NULL);
	assert(Q_GET_FRONT(&list1) == &node[0]);
	assert(Q_GET_TAIL(&list1) == &node[0]);
	assert(!Q_GET_NEXT(&node[0], link));
	assert(!Q_GET_PREV(&node[0], link));


	/* Prefix empty */
	Q_APPEND(&list1, &list2, link);
	assert(Q_GET_FRONT(&list2) == NULL);
	assert(Q_GET_TAIL(&list2) == NULL);
	assert(Q_GET_FRONT(&list1) == &node[0]);
	assert(Q_GET_TAIL(&list1) == &node[0]);
	assert(!Q_GET_NEXT(&node[0], link));
	assert(!Q_GET_PREV(&node[0], link));

	/* Both non-empty */
	Q_INSERT_TAIL(&list2, &node[1], link);
	Q_APPEND(&list1, &list2, link);
	assert(Q_GET_FRONT(&list2) == NULL);
	assert(Q_GET_TAIL(&list2) == NULL);
	assert(Q_GET_FRONT(&list1) == &node[0]);
	assert(Q_GET_TAIL(&list1) == &node[1]);
	assert(Q_GET_NEXT(&node[0], link) == &node[1]);
	assert(Q_GET_PREV(&node[1], link) == &node[0]);
	assert(Q_GET_NEXT(&node[1], link) == NULL);
	assert(Q_GET_PREV(&node[0], link) == NULL);

	printf("PASSED Q_APPEND vq_test!");
}




int
main()
{
	test_append();
	exit(0);
}


