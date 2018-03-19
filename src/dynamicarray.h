/* An implementation of a resizing array. */

#ifndef DYNAMICARRAY_H
#define DYNAMICARRAY_H

#include <stddef.h>

struct DynamicArray {
	size_t len;
	size_t maxlen;
	void **values;
};

/* Create a new, empty dynamic array.

This returns `NULL` when there's no memory.
*/
struct DynamicArray *dynamicarray_new(void);

/* This should be called when the array is not needed anymore. */
void dynamicarray_free(struct DynamicArray *arr);

/* Call `freefunc(item)` for each item in the array and then call `dynamicarray_free`. */
void dynamicarray_freeall(struct DynamicArray *arr, void (*freefunc)(void *));

/* Add an item to the array, resizing it if needed.

This returns `0` on success and `-1` when there's not enough memory.
*/
int dynamicarray_push(struct DynamicArray *arr, void *obj);

/* Pop an item from the end of the array and return it. */
void *dynamicarray_pop(struct DynamicArray *arr);

/* Check if two dynamic arrays are equal as defined by `cmpfunc`.

`cmpfunc` should usually return `1` for equal, `0` for not equal or a negative
number for error. Note that this is different from e.g. `strcmp()`.

If the lengths of the arrays differ, this returns `0` right away. Otherwise
this calls `cmpfunc(item1, item2)` for every pair of items in the arrays, and
if `cmpfunc` returns something non-positive, this returns that value right
away. If `cmpfunc` returned a positive value for every pair, this returns `1`.
*/
int dynamicarray_equals(struct DynamicArray *arr1, struct DynamicArray *arr2, int (*cmpfunc)(void *, void *));

#endif   // DYNAMICARRAY_H
