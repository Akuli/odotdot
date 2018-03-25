#include <stdlib.h>

#include "utils.h"
#include <src/dynamicarray.h>

int working_cmpfunc(void *x, void *y) { return *((int *)x) == *((int *)y); }
int yesyes_cmpfunc(void *x, void *y) { return 1; }
int error_cmpfunc(void *x, void *y) { return -2; }


void test_dynamiarray_new_push_pop_and_freeall(void)
{
	struct DynamicArray *arr = dynamicarray_new();
	buttert(arr->len == 0);

	for (int i=0; i < 100; i++) {
		int *ip = bmalloc(sizeof(int));
		*ip = i;
		dynamicarray_push(arr, ip);
	}
	buttert(arr->len == 100);

	for (int i=99; i >= 50; i--) {
		int *ip = dynamicarray_pop(arr);
		buttert(*ip == i);
		free(ip);
	}
	buttert(arr->len == 50);

	for (int i=49; i>=0; i--) {
		free(dynamicarray_pop(arr));
	}
	buttert(arr->len == 0);
	dynamicarray_free(arr);
}

void test_dynamiarray_equals_and_free(void)
{
	struct DynamicArray *arr1 = dynamicarray_new();
	struct DynamicArray *arr2 = dynamicarray_new();
	buttert(arr1 && arr2);

	for (int i=0; i<5; i++) {
		int *ip1 = bmalloc(sizeof(int));
		int *ip2 = bmalloc(sizeof(int));
		*ip1 = *ip2 = i;
		buttert(dynamicarray_push(arr1, ip1) == 0);
		buttert(dynamicarray_push(arr2, ip2) == 0);
	}

	buttert(dynamicarray_equals(arr1, arr2, working_cmpfunc) == 1);
	buttert(dynamicarray_equals(arr1, arr2, yesyes_cmpfunc) == 1);
	buttert(dynamicarray_equals(arr1, arr2, error_cmpfunc) == -2);

	int *ip = bmalloc(sizeof(int));
	*ip = 1;
	buttert(dynamicarray_push(arr1, ip) == 0);
	buttert(dynamicarray_equals(arr1, arr2, working_cmpfunc) == 0);
	buttert(dynamicarray_equals(arr1, arr2, yesyes_cmpfunc) == 0);
	buttert(dynamicarray_equals(arr1, arr2, error_cmpfunc) == 0);

	ip = bmalloc(sizeof(int));
	*ip = 2;
	dynamicarray_push(arr2, ip);
	buttert(dynamicarray_equals(arr1, arr2, working_cmpfunc) == 0);
	buttert(dynamicarray_equals(arr1, arr2, yesyes_cmpfunc) == 1);
	buttert(dynamicarray_equals(arr1, arr2, error_cmpfunc) == -2);

	dynamicarray_freeall(arr1, free);
	dynamicarray_freeall(arr2, free);
}
