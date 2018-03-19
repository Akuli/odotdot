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
	size_t new_maxlen = arr->maxlen*2;
	void **ptr = realloc(arr->values, new_maxlen * sizeof (void *));
	if (!ptr)
		return 1;
	arr->maxlen = new_maxlen;
	arr->values = ptr;
	return 0;
}

int dynamicarray_push(struct DynamicArray *arr, void *obj)
{
	if (arr->len + 1 > arr->maxlen) {
		if (resize(arr))
			return 1;
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

void dynamicarray_freeall(struct DynamicArray *arr, void (*free_func)(void *))
{
	for (size_t i=0; i < arr->len; i++)
		free_func(arr->values[i]);
	dynamicarray_free(arr);
}
