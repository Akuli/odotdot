#include "array.h"
#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include "classobject.h"
#include "string.h"
#include "../common.h"
#include "../dynamicarray.h"
#include "../interpreter.h"
#include "../method.h"
#include "../objectsystem.h"

static void array_foreachref(struct Object *obj, void *data, objectclassinfo_foreachrefcb cb)
{
	struct DynamicArray *dynarr = obj->data;
	for (size_t i=0; i < dynarr->len; i++)
		cb(dynarr->values[i], data);
}

static void array_destructor(struct Object *arr)
{
	dynamicarray_free(arr->data);
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
static struct Object *to_string_joiner(struct Interpreter *interp, struct Object **errptr, struct Object **strings, size_t nstrings)
{
	// first compute total length, then malloc at once, should be more efficient than many reallocs
	// TODO: check for overflow? the length may be huge if the array is huge and elements have huge reprs
	struct UnicodeString ustr;
	ustr.len = 0;
	for (size_t i=0; i < nstrings; i++)
		ustr.len += ((struct UnicodeString *) strings[i]->data)->len;
	ustr.len += nstrings - 1;   // spaces between the items
	ustr.len += 2;     // [ ]

	ustr.val = malloc(sizeof(uint32_t) * ustr.len);
	if (!ustr.val) {
		*errptr = interp->nomemerr;
		return NULL;
	}

	uint32_t *ptr = ustr.val;
	*ptr++ = '[';
	for (size_t i=0; i < nstrings; i++) {
		struct UnicodeString *part = strings[i]->data;
		memcpy(ptr, part->val, sizeof(uint32_t) * part->len);
		ptr += part->len;
		*ptr++ = (i==nstrings-1 ? ']' : ' ');
	}

	// TODO: this copies res and it might be very big, add something like stringobject_newfromustr_nocopy
	struct Object *res = stringobject_newfromustr(interp, errptr, ustr);
	free(ustr.val);
	return res;
}

static struct Object *to_string(struct Context *ctx, struct Object **errptr, struct Object **args, size_t nargs)
{
	if (functionobject_checktypes(ctx, errptr, args, nargs, "Array", NULL) == STATUS_ERROR)
		return NULL;

	struct DynamicArray *dynarr = args[0]->data;

	// this is handeled specially because malloc(0) may return NULL
	if (dynarr->len == 0)
		return stringobject_newfromcharptr(ctx->interp, errptr, "[]");

	struct Object **strings = malloc(sizeof(struct Object*) * dynarr->len);
	if (!strings) {
		*errptr = ctx->interp->nomemerr;
		return NULL;
	}

	struct Object *stringclass = interpreter_getbuiltin(ctx->interp, errptr, "String");
	for (size_t i = 0; i < dynarr->len; i++) {
		struct Object *stringed = method_call(ctx, errptr, dynarr->values[i], "to_debug_string", NULL);
		if (!stringed) {
			for (size_t j=0; j<i; j++)
				OBJECT_DECREF(ctx->interp, strings[i]);
			free(strings);
			OBJECT_DECREF(ctx->interp, stringclass);
			return NULL;
		}
		assert(stringed->klass == stringclass);    // TODO: better error handling
		strings[i] = stringed;
	}
	OBJECT_DECREF(ctx->interp, stringclass);

	struct Object *res = to_string_joiner(ctx->interp, errptr, strings, dynarr->len);
	for (size_t i=0; i < dynarr->len; i++)
		OBJECT_DECREF(ctx->interp, strings[i]);
	free(strings);
	return res;
}

int arrayobject_createclass(struct Interpreter *interp, struct Object **errptr)
{
	struct Object *objectclass = interpreter_getbuiltin(interp, errptr, "Object");
	if (!objectclass)
		return STATUS_ERROR;

	struct Object *klass = classobject_new(interp, errptr, "Array", objectclass, array_foreachref, array_destructor);
	OBJECT_DECREF(interp, objectclass);
	if (!klass)
		return STATUS_ERROR;

	if (method_add(interp, errptr, klass, "to_string", to_string) == STATUS_ERROR) {
		OBJECT_DECREF(interp, klass);
		return STATUS_ERROR;
	}

	int res = interpreter_addbuiltin(interp, errptr, "Array", klass);
	OBJECT_DECREF(interp, klass);
	return res;
}

// TODO: add something for creating DynamicArrays from existing item arrays efficiently
struct Object *arrayobject_newempty(struct Interpreter *interp, struct Object **errptr)
{
	struct DynamicArray *dynarr = dynamicarray_new();
	if (!dynarr)
		return NULL;

	struct Object *klass = interpreter_getbuiltin(interp, errptr, "Array");
	if (!klass) {
		dynamicarray_free(dynarr);
		return NULL;
	}

	struct Object *arr = classobject_newinstance(interp, errptr, klass, dynarr);
	OBJECT_DECREF(interp, klass);
	if (!arr) {
		dynamicarray_free(dynarr);
		return NULL;
	}
	return arr;
}

struct Object *arrayobject_new(struct Interpreter *interp, struct Object **errptr, struct Object **elems, size_t nelems)
{
	struct Object *arr = arrayobject_newempty(interp, errptr);
	if (!arr)
		return NULL;

	for (size_t i=0; i < nelems; i++) {
		int status = dynamicarray_push(arr->data, elems[i]);
		if (status != STATUS_OK) {
			assert(status == STATUS_NOMEM);
			*errptr = interp->nomemerr;
			for (size_t j=0; j<i; j++)
				OBJECT_DECREF(interp, elems[i]);
			OBJECT_DECREF(interp, arr);
			return NULL;
		}
		OBJECT_INCREF(interp, elems[i]);
	}

	return arr;
}
