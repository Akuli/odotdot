// an array implementation that resizes nicely

#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>

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

static int resize(struct DynamicArray *arr) {
	arr->maxlen *= 2;
	arr->values = realloc(arr->values, arr->maxlen * sizeof (void *));
	if (!(arr->values))
		return 1;
	return 0;
}

int dynamicarray_push(struct DynamicArray *arr, void *obj)
{
	arr->len++;
	if (arr->len > arr->maxlen) {
		int error=resize(arr);
		if (error)
			return error;
	}
	arr->values[arr->len-1] = obj;
	return 0;
}

void *dynamicarray_pop(struct DynamicArray *arr)
{
	arr->len--;
	void *obj = arr->values[arr->len];
	return obj;
}

void dynamicarray_free2(struct DynamicArray *arr, void (*free_func)(void *))
{
	while (arr->len)
		free_func(dynamicarray_pop(arr));
	free(arr->values);
	free(arr);
}
