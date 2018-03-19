#include "utils.h"
#include <src/dynamicarray.h>

BEGIN_TESTS

TEST(dynamicarray) {
	struct DynamicArray *arr = dynamicarray_new();
	buttert(arr->len == 0);

	for (int i=1; i <= 100; i++) {
		int *ip = malloc(sizeof(int));
		buttert(ip);
		*ip = i;
		dynamicarray_push(arr, ip);
	}
	buttert(arr->len == 100);

	for (int i=100; i > 50; i--) {
		int *ip = dynamicarray_pop(arr);
		buttert(*ip == i);
		free(ip);
	}
	buttert(arr->len == 50);

	dynamicarray_freeall(arr, free);
	// cannot check arr->len anymore, arr has been freed
}

END_TESTS
