#ifndef DYNAMICARRAY_H
#define DYNAMICARRAY_H

struct DynamicArray {
	size_t len;
	size_t maxlen;
	void **values;
};

struct DynamicArray *dynamicarray_new(void);
void dynamicarray_free(struct DynamicArray *arr);
void dynamicarray_freeall(struct DynamicArray *arr, void (*free_func)(void *));
int dynamicarray_push(struct DynamicArray *arr, void *obj);
void *dynamicarray_pop(struct DynamicArray *arr);

#endif   // DYNAMICARRAY_H
