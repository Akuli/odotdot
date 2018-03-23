// an array implementation that resizes nicely

#include <stddef.h>
#include <stdlib.h>
#include "common.h"
#include "dynamicarray.h"


struct DynamicArray *dynamicarray_new(void)
{
	struct DynamicArray *arr = malloc(sizeof (struct DynamicArray));
	arr->len = 0;
	arr->maxlen = 3;
	arr->values = malloc(arr->maxlen * sizeof (void *));
	if (arr->values == NULL) {
		free(arr);
		return NULL;
	}
	return arr;
}

void dynamicarray_free(struct DynamicArray *arr)
{
	free(arr->values);
	free(arr);
}

void dynamicarray_freeall(struct DynamicArray *arr, void (*freefunc)(void *))
{
	for (size_t i=0; i < arr->len; i++)
		freefunc(arr->values[i]);
	dynamicarray_free(arr);
}

static int resize(struct DynamicArray *arr) {
	size_t new_maxlen = arr->maxlen*2;
	void **ptr = realloc(arr->values, new_maxlen * sizeof (void *));
	if (!ptr)
		return STATUS_NOMEM;
	arr->maxlen = new_maxlen;
	arr->values = ptr;
	return STATUS_OK;
}

int dynamicarray_push(struct DynamicArray *arr, void *obj)
{
	if (arr->len + 1 > arr->maxlen) {
		int status = resize(arr);
		if (status != STATUS_OK)
			return status;
	}
	arr->len++;
	arr->values[arr->len-1] = obj;
	return 0;
}

void *dynamicarray_pop(struct DynamicArray *arr)
{
	arr->len--;
	void *obj = arr->values[arr->len];
	return obj;
}

int dynamicarray_equals(struct DynamicArray *arr1, struct DynamicArray *arr2, int (*cmpfunc)(void *, void *))
{
	if (arr1->len != arr2->len)
		return 0;

	for (size_t i=0; i < arr1->len; i++) {
		int res = cmpfunc(arr1->values[i], arr2->values[i]);

		// if res == 0, the items are not equal
		// if res < 0, an error occurred
		if (res <= 0)
			return res;
	}
	return 1;
}
