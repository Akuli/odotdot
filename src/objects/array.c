#include "array.h"
#include <limits.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include "../check.h"
#include "../common.h"
#include "../interpreter.h"
#include "../method.h"
#include "../objectsystem.h"
#include "../unicode.h"
#include "classobject.h"
#include "errors.h"
#include "function.h"
#include "integer.h"
#include "string.h"

static void array_foreachref(struct Object *arr, void *cbdata, classobject_foreachrefcb cb)
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


static struct Object *setup(struct Interpreter *interp, struct Object *argarr)
{
	errorobject_setwithfmt(interp, "arrays can't be created with (new Array), use [ ] instead");
	return NULL;
}

/* joins string objects together and adds [ ] around the whole thing
to_string was becoming huge, had to break into two functions
TODO: how about putting the array inside itself? python does this:
>>> asd = []
>>> asd = [1, 2, 3]
>>> asd.append(asd)
>>> asd
[1, 2, 3, [...]]

python's reprlib.recursive_repr() handles this, maybe check how it's implemented? */
static struct Object *to_string_joiner(struct Interpreter *interp, struct Object **strings, size_t nstrings)
{
	// first compute total length, then malloc at once, should be more efficient than many reallocs
	// TODO: check for overflow? the length may be huge if the array is huge and elements have huge reprs
	struct UnicodeString ustr;
	ustr.len = 0;
	for (size_t i=0; i < nstrings; i++)
		ustr.len += ((struct UnicodeString *) strings[i]->data)->len;
	ustr.len += nstrings - 1;   // spaces between the items
	ustr.len += 2;     // [ ]

	ustr.val = malloc(sizeof(unicode_char) * ustr.len);
	if (!ustr.val) {
		errorobject_setnomem(interp);
		return NULL;
	}

	unicode_char *ptr = ustr.val;
	*ptr++ = '[';
	for (size_t i=0; i < nstrings; i++) {
		struct UnicodeString *part = strings[i]->data;
		memcpy(ptr, part->val, sizeof(unicode_char) * part->len);
		ptr += part->len;
		*ptr++ = (i==nstrings-1 ? ']' : ' ');
	}

	// TODO: this copies res and it might be very big, add something like stringobject_newfromustr_nocopy
	struct Object *res = stringobject_newfromustr(interp, ustr);
	free(ustr.val);
	return res;
}

static struct Object *to_string(struct Interpreter *interp, struct Object *argarr)
{
	if (check_args(interp, argarr, interp->builtins.arrayclass, NULL) == STATUS_ERROR)
		return NULL;
	struct Object *arr = ARRAYOBJECT_GET(argarr, 0);

	// this is handeled specially because malloc(0) may return NULL
	if (ARRAYOBJECT_LEN(arr) == 0)
		return stringobject_newfromcharptr(interp, "[]");

	// TODO: check for overflow with * or use calloc?
	struct Object **strings = malloc(sizeof(struct Object*) * ARRAYOBJECT_LEN(arr));
	if (!strings) {
		errorobject_setnomem(interp);
		return NULL;
	}

	for (size_t i = 0; i < ARRAYOBJECT_LEN(arr); i++) {
		struct Object *stringed = method_call_todebugstring(interp, ARRAYOBJECT_GET(arr, i));
		if (!stringed) {
			for (size_t j=0; j<i; j++)
				OBJECT_DECREF(interp, strings[i]);
			free(strings);
			return NULL;
		}
		strings[i] = stringed;
	}

	struct Object *res = to_string_joiner(interp, strings, ARRAYOBJECT_LEN(arr));
	for (size_t i=0; i < ARRAYOBJECT_LEN(arr); i++)
		OBJECT_DECREF(interp, strings[i]);
	free(strings);
	return res;
}

static int validate_index(struct Interpreter *interp, struct Object *arr, long long i)
{
	if (i < 0) {
		errorobject_setwithfmt(interp, "%L is not a valid array index", i);
		return STATUS_ERROR;
	}
	if ((unsigned long long) i >= ARRAYOBJECT_LEN(arr)) {
		errorobject_setwithfmt(interp, "%L is not a valid index for an array of length %L", i, (long long) ARRAYOBJECT_LEN(arr));
		return STATUS_ERROR;
	}
	return STATUS_OK;
}

static struct Object *get(struct Interpreter *interp, struct Object *argarr)
{
	if (check_args(interp, argarr, interp->builtins.arrayclass, interp->builtins.integerclass, NULL) == STATUS_ERROR)
		return NULL;
	struct Object *arr = ARRAYOBJECT_GET(argarr, 0);
	struct Object *index = ARRAYOBJECT_GET(argarr, 1);

	long long i = integerobject_tolonglong(index);
	if (validate_index(interp, arr, i) == STATUS_ERROR)
		return NULL;

	struct Object *res = ARRAYOBJECT_GET(arr, i);
	OBJECT_INCREF(interp, res);
	return res;
}

static struct Object *set(struct Interpreter *interp, struct Object *argarr)
{
	if (check_args(interp, argarr, interp->builtins.arrayclass, interp->builtins.integerclass, interp->builtins.objectclass, NULL) == STATUS_ERROR)
		return NULL;
	struct Object *arr = ARRAYOBJECT_GET(argarr, 0);
	struct Object *index = ARRAYOBJECT_GET(argarr, 1);
	struct Object *obj = ARRAYOBJECT_GET(argarr, 2);

	long long i = integerobject_tolonglong(index);
	if (validate_index(interp, arr, i) == STATUS_ERROR)
		return NULL;

	struct ArrayObjectData *data = arr->data;
	OBJECT_DECREF(interp, data->elems[i]);
	data->elems[i] = obj;
	OBJECT_INCREF(interp, obj);

	return interpreter_getbuiltin(interp, "null");
}

static struct Object *push(struct Interpreter *interp, struct Object *argarr)
{
	if (check_args(interp, argarr, interp->builtins.arrayclass, interp->builtins.objectclass, NULL) == STATUS_ERROR)
		return NULL;
	struct Object *arr = ARRAYOBJECT_GET(argarr, 0);
	struct Object *obj = ARRAYOBJECT_GET(argarr, 1);

	if (arrayobject_push(interp, arr, obj) == STATUS_ERROR)
		return NULL;
	return interpreter_getbuiltin(interp, "null");
}

static struct Object *pop(struct Interpreter *interp, struct Object *argarr)
{
	if (check_args(interp, argarr, interp->builtins.arrayclass, NULL) == STATUS_ERROR)
		return NULL;

	struct Object *res = arrayobject_pop(interp, ARRAYOBJECT_GET(argarr, 0));
	if (!res)
		errorobject_setwithfmt(interp, "cannot pop from an empty array");
	return res;
}

static struct Object *slice(struct Interpreter *interp, struct Object *argarr)
{
	long long i, j;
	if (ARRAYOBJECT_LEN(argarr) == 2) {
		// (thing.slice i) is same as (thing.slice i (thing.get_length))
		if (check_args(interp, argarr, interp->builtins.arrayclass, interp->builtins.integerclass, NULL) == STATUS_ERROR)
			return NULL;
		i = integerobject_tolonglong(ARRAYOBJECT_GET(argarr, 1));
		j = LLONG_MAX;
	} else {
		if (check_args(interp, argarr, interp->builtins.arrayclass, interp->builtins.integerclass, interp->builtins.integerclass, NULL) == STATUS_ERROR)
			return NULL;
		i = integerobject_tolonglong(ARRAYOBJECT_GET(argarr, 1));
		j = integerobject_tolonglong(ARRAYOBJECT_GET(argarr, 2));
	}
	return arrayobject_slice(interp, ARRAYOBJECT_GET(argarr, 0), i, j);
}

// TODO: should this be an attribute?
static struct Object *get_length(struct Interpreter *interp, struct Object *argarr)
{
	if (check_args(interp, argarr, interp->builtins.arrayclass, NULL) == STATUS_ERROR)
		return NULL;
	return integerobject_newfromlonglong(interp, ARRAYOBJECT_LEN(ARRAYOBJECT_GET(argarr, 0)));
}

struct Object *arrayobject_createclass(struct Interpreter *interp)
{
	struct Object *klass = classobject_new(interp, "Array", interp->builtins.objectclass, array_foreachref);
	if (!klass)
		return NULL;

	if (method_add(interp, klass, "setup", setup) == STATUS_ERROR) goto error;
	if (method_add(interp, klass, "set", set) == STATUS_ERROR) goto error;
	if (method_add(interp, klass, "get", get) == STATUS_ERROR) goto error;
	if (method_add(interp, klass, "push", push) == STATUS_ERROR) goto error;
	if (method_add(interp, klass, "pop", pop) == STATUS_ERROR) goto error;
	if (method_add(interp, klass, "slice", slice) == STATUS_ERROR) goto error;
	if (method_add(interp, klass, "get_length", get_length) == STATUS_ERROR) goto error;
	if (method_add(interp, klass, "to_string", to_string) == STATUS_ERROR) goto error;
	return klass;

error:
	OBJECT_DECREF(interp, klass);
	return NULL;
}

struct Object *arrayobject_new(struct Interpreter *interp, struct Object **elems, size_t nelems)
{
	struct ArrayObjectData *data = malloc(sizeof(struct ArrayObjectData));
	if (!data) {
		errorobject_setnomem(interp);
		return NULL;
	}

	data->len = nelems;
	if (nelems <= 3)   // 3 feels good
		data->nallocated = 3;
	else
		data->nallocated = nelems;

	data->elems = malloc(data->nallocated * sizeof(struct Object*));
	if (nelems > 0 && !(data->elems)) {   // malloc(0) MAY be NULL
		errorobject_setnomem(interp);
		free(data);
		return NULL;
	}

	struct Object *arr = classobject_newinstance(interp, interp->builtins.arrayclass, data, array_destructor);
	if (!arr) {
		free(data->elems);
		free(data);
		return NULL;
	}

	// rest of this can't fail
	// TODO: which is more efficient, memcpy and a loop or 1 loop and assignments?
	memcpy(data->elems, elems, sizeof(struct Object *) * nelems);
	for (size_t i=0; i < nelems; i++)
		OBJECT_INCREF(interp, elems[i]);

	arr->hashable = 0;
	return arr;
}

static int resize(struct Interpreter *interp, struct ArrayObjectData *data)
{
	// allocating more than this is not possible because realloc takes size_t
#define NALLOCATED_MAX (SIZE_MAX / sizeof(struct Object*))

	if (data->nallocated == NALLOCATED_MAX) {
		// a very big array
		// this is not the best possible error message but not too bad IMO
		// nobody will actually have this happening anyway
		errorobject_setnomem(interp);
		return STATUS_ERROR;
	}

	size_t newnallocated = data->nallocated > NALLOCATED_MAX/2 ? NALLOCATED_MAX : 2*data->nallocated;
	void *ptr = realloc(data->elems, newnallocated * sizeof(struct Object*));
	if (!ptr) {
		errorobject_setnomem(interp);
		return STATUS_ERROR;
	}

	data->elems = ptr;
	data->nallocated = newnallocated;
	return STATUS_OK;
}

int arrayobject_push(struct Interpreter *interp, struct Object *arr, struct Object *obj)
{
	struct ArrayObjectData *data = arr->data;
	if (data->len + 1 > data->nallocated) {
		if (resize(interp, data) == STATUS_ERROR)
			return STATUS_ERROR;
	}

	OBJECT_INCREF(interp, obj);
	data->elems[data->len++] = obj;
	return STATUS_OK;
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
		if (arrayobject_push(interp, res, ARRAYOBJECT_GET(arr, i)) == STATUS_ERROR) {
			OBJECT_DECREF(interp, res);
			return NULL;
		}
	}
	return res;
}
