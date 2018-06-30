#include "array.h"
#include <assert.h>
#include <limits.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include "../attribute.h"
#include "../check.h"
#include "../interpreter.h"
#include "../method.h"
#include "../objectsystem.h"
#include "../operator.h"
#include "../unicode.h"
#include "bool.h"
#include "classobject.h"
#include "errors.h"
#include "function.h"
#include "integer.h"
#include "null.h"
#include "string.h"

static void array_foreachref(struct Object *arr, void *cbdata, object_foreachrefcb cb)
{
	struct ArrayObjectData *data = arr->data;
	for (size_t i=0; i < data->len; i++)
		cb(data->elems[i], cbdata);
}

static void array_destructor(struct Object *arr)
{
	// the elements have already been decreffed because array_foreachref
	struct ArrayObjectData *data = arr->data;
	free(data->elems);
	free(data);
}


static struct Object *newinstance(struct Interpreter *interp, struct Object *args, struct Object *opts)
{
	errorobject_throwfmt(interp, "TypeError", "arrays can't be created with (new Array), use [ ] instead");
	return NULL;
}

static bool validate_index(struct Interpreter *interp, struct Object *arr, long long i)
{
	if (i < 0) {
		errorobject_throwfmt(interp, "ValueError", "%L is not a valid array index", i);
		return false;
	}
	if ((unsigned long long) i >= ARRAYOBJECT_LEN(arr)) {
		errorobject_throwfmt(interp, "ValueError", "%L is not a valid index for an array of length %L", i, (long long) ARRAYOBJECT_LEN(arr));
		return false;
	}
	return true;
}

static struct Object *get(struct Interpreter *interp, struct Object *args, struct Object *opts)
{
	if (!check_args(interp, args, interp->builtins.Array, interp->builtins.Integer, NULL)) return NULL;
	if (!check_no_opts(interp, opts)) return NULL;
	struct Object *arr = ARRAYOBJECT_GET(args, 0);
	struct Object *index = ARRAYOBJECT_GET(args, 1);

	long long i = integerobject_tolonglong(index);
	if (!validate_index(interp, arr, i))
		return NULL;

	struct Object *res = ARRAYOBJECT_GET(arr, i);
	OBJECT_INCREF(interp, res);
	return res;
}

static struct Object *set(struct Interpreter *interp, struct Object *args, struct Object *opts)
{
	if (!check_args(interp, args, interp->builtins.Array, interp->builtins.Integer, interp->builtins.Object, NULL)) return NULL;
	if (!check_no_opts(interp, opts)) return NULL;
	struct Object *arr = ARRAYOBJECT_GET(args, 0);
	struct Object *index = ARRAYOBJECT_GET(args, 1);
	struct Object *obj = ARRAYOBJECT_GET(args, 2);

	long long i = integerobject_tolonglong(index);
	if (!validate_index(interp, arr, i))
		return NULL;

	struct ArrayObjectData *data = arr->data;
	OBJECT_DECREF(interp, data->elems[i]);
	data->elems[i] = obj;
	OBJECT_INCREF(interp, obj);

	return nullobject_get(interp);
}

static struct Object *push(struct Interpreter *interp, struct Object *args, struct Object *opts)
{
	if (!check_args(interp, args, interp->builtins.Array, interp->builtins.Object, NULL)) return NULL;
	if (!check_no_opts(interp, opts)) return NULL;
	struct Object *arr = ARRAYOBJECT_GET(args, 0);
	struct Object *obj = ARRAYOBJECT_GET(args, 1);

	if (!arrayobject_push(interp, arr, obj))
		return NULL;
	return nullobject_get(interp);
}

static struct Object *pop(struct Interpreter *interp, struct Object *args, struct Object *opts)
{
	if (!check_args(interp, args, interp->builtins.Array, NULL)) return NULL;
	if (!check_no_opts(interp, opts)) return NULL;

	struct Object *res = arrayobject_pop(interp, ARRAYOBJECT_GET(args, 0));
	if (!res)
		errorobject_throwfmt(interp, "ValueError", "cannot pop from an empty array");
	return res;
}

static struct Object *slice(struct Interpreter *interp, struct Object *args, struct Object *opts)
{
	if (!check_no_opts(interp, opts))
		return NULL;

	long long i, j;
	if (ARRAYOBJECT_LEN(args) == 2) {
		// (thing.slice i) is same as (thing.slice i thing.length)
		if (!check_args(interp, args, interp->builtins.Array, interp->builtins.Integer, NULL))
			return NULL;
		i = integerobject_tolonglong(ARRAYOBJECT_GET(args, 1));
		j = ARRAYOBJECT_LEN(ARRAYOBJECT_GET(args, 0));
	} else {
		if (!check_args(interp, args, interp->builtins.Array, interp->builtins.Integer, interp->builtins.Integer, NULL))
			return NULL;
		i = integerobject_tolonglong(ARRAYOBJECT_GET(args, 1));
		j = integerobject_tolonglong(ARRAYOBJECT_GET(args, 2));
	}
	return arrayobject_slice(interp, ARRAYOBJECT_GET(args, 0), i, j);
}

static struct Object *length_getter(struct Interpreter *interp, struct Object *args, struct Object *opts)
{
	if (!check_args(interp, args, interp->builtins.Array, NULL)) return NULL;
	if (!check_no_opts(interp, opts)) return NULL;
	return integerobject_newfromlonglong(interp, ARRAYOBJECT_LEN(ARRAYOBJECT_GET(args, 0)));
}

struct Object *arrayobject_createclass(struct Interpreter *interp)
{
	struct Object *klass = classobject_new(interp, "Array", interp->builtins.Object, newinstance);
	if (!klass)
		return NULL;

	// TODO: figure out better names for push and pop?
	if (!attribute_add(interp, klass, "length", length_getter, NULL)) goto error;
	if (!method_add(interp, klass, "set", set)) goto error;
	if (!method_add(interp, klass, "get", get)) goto error;
	if (!method_add(interp, klass, "push", push)) goto error;
	if (!method_add(interp, klass, "pop", pop)) goto error;
	if (!method_add(interp, klass, "slice", slice)) goto error;
	return klass;

error:
	OBJECT_DECREF(interp, klass);
	return NULL;
}


struct Object *arrayobject_new(struct Interpreter *interp, struct Object **elems, size_t nelems)
{
	struct ArrayObjectData *data = malloc(sizeof(struct ArrayObjectData));
	if (!data) {
		errorobject_thrownomem(interp);
		return NULL;
	}

	data->len = nelems;
	if (nelems <= 3)   // 3 feels good, can't be 0 because malloc(0) may return NULL on success
		data->nallocated = 3;
	else
		data->nallocated = nelems;

	data->elems = malloc(data->nallocated * sizeof(struct Object*));
	if (!data->elems) {
		errorobject_thrownomem(interp);
		free(data);
		return NULL;
	}

	struct Object *arr = object_new_noerr(interp, interp->builtins.Array, data, array_foreachref, array_destructor);
	if (!arr) {
		errorobject_thrownomem(interp);
		free(data->elems);
		free(data);
		return NULL;
	}

	// rest of this can't fail
	// TODO: which is more efficient, memcpy and a loop or 1 loop and assignments?
	memcpy(data->elems, elems, sizeof(struct Object *) * nelems);
	for (size_t i=0; i < nelems; i++)
		OBJECT_INCREF(interp, elems[i]);

	arr->hashable = false;
	return arr;
}

struct Object *arrayobject_concat(struct Interpreter *interp, struct Object *arr1, struct Object *arr2)
{
	struct ArrayObjectData *data1 = arr1->data;
	struct ArrayObjectData *data2 = arr2->data;
	struct ArrayObjectData *data = malloc(sizeof(struct ArrayObjectData));
	if (!data) {
		errorobject_thrownomem(interp);
		return NULL;
	}

	data->len = data1->len + data2->len;
	if (data->len <= 3)   // 3 feels good, can't be 0 because malloc(0) may return NULL on success
		data->nallocated = 3;
	else
		data->nallocated = data->len;

	data->elems = malloc(data->nallocated * sizeof(struct Object*));
	if (!data->elems) {
		errorobject_thrownomem(interp);
		free(data);
		return NULL;
	}

	struct Object *arr = object_new_noerr(interp, interp->builtins.Array, data, array_foreachref, array_destructor);
	if (!arr) {
		errorobject_thrownomem(interp);
		free(data->elems);
		free(data);
		return NULL;
	}

	// rest of this can't fail
	memcpy(data->elems, data1->elems, sizeof(struct Object*) * data1->len);
	memcpy(data->elems + data1->len, data2->elems, sizeof(struct Object*) * data2->len);
	for (size_t i=0; i < data->len; i++)
		OBJECT_INCREF(interp, data->elems[i]);

	arr->hashable = false;
	return arr;
}

static bool resize(struct Interpreter *interp, struct ArrayObjectData *data)
{
	assert(data->nallocated > 0);

	// allocating more than this is not possible because realloc takes size_t
#define NALLOCATED_MAX (SIZE_MAX / sizeof(struct Object*))

	if (data->nallocated == NALLOCATED_MAX) {
		// a very big array
		// this is not the best possible error message but not too bad IMO
		// nobody will actually have this happening anyway
		errorobject_thrownomem(interp);
		return false;
	}

	size_t newnallocated = data->nallocated > NALLOCATED_MAX/2 ? NALLOCATED_MAX : 2*data->nallocated;
	void *ptr = realloc(data->elems, newnallocated * sizeof(struct Object*));
	if (!ptr) {
		errorobject_thrownomem(interp);
		return false;
	}

	data->elems = ptr;
	data->nallocated = newnallocated;
	return true;
}

bool arrayobject_push(struct Interpreter *interp, struct Object *arr, struct Object *obj)
{
	struct ArrayObjectData *data = arr->data;
	if (data->len + 1 > data->nallocated) {
		if (!resize(interp, data))
			return false;
	}

	OBJECT_INCREF(interp, obj);
	data->elems[data->len++] = obj;
	return true;
}

struct Object *arrayobject_pop(struct Interpreter *interp, struct Object *arr)
{
	struct ArrayObjectData *data = arr->data;
	if (data->len == 0)
		return NULL;

	// don't touch refcounts, we remove a reference from the data but we also return a reference
	return data->elems[--data->len];
}

struct Object *arrayobject_slice(struct Interpreter *interp, struct Object *arr, long long start, long long end)
{
	// ignore out of bounds indexes, like in python
	if (start < 0)
		start = 0;
	if (end < 0)
		return arrayobject_newempty(interp);   // may be NULL
	// now end is guaranteed to be non-negative, so it can be casted to size_t
	if ((size_t)end > ARRAYOBJECT_LEN(arr))
		end = ARRAYOBJECT_LEN(arr);
	if (start >= end)
		return arrayobject_newempty(interp);   // may be NULL

	struct Object *res = arrayobject_newempty(interp);
	if (!res)
		return NULL;

	for (size_t i=start; i < (size_t) end; i++) {
		if (!arrayobject_push(interp, res, ARRAYOBJECT_GET(arr, i))) {
			OBJECT_DECREF(interp, res);
			return NULL;
		}
	}
	return res;
}
