#include "bytearray.h"
#include <assert.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include "../attribute.h"
#include "../check.h"
#include "../interpreter.h"
#include "../method.h"
#include "../objectsystem.h"
#include "array.h"
#include "classobject.h"
#include "errors.h"
#include "integer.h"
#include "option.h"

static void bytearray_destructor(void *data)
{
	free(((struct ByteArrayObjectData *)data)->val);
	free(data);
}


static struct Object *newinstance(struct Interpreter *interp, struct Object *args, struct Object *opts)
{
	if (!check_args(interp, args, interp->builtins.Class, interp->builtins.Array, NULL)) return NULL;
	if (!check_no_opts(interp, opts)) return NULL;
	struct Object *klass = ARRAYOBJECT_GET(args, 0);
	struct Object *arr = ARRAYOBJECT_GET(args, 1);

	struct ByteArrayObjectData *data = malloc(sizeof *data);
	if (!data)
	{
		errorobject_thrownomem(interp);
		return NULL;
	}

	// malloc(0) may return NULL
	if (ARRAYOBJECT_LEN(arr) == 0) {
		data->len = 0;
		data->val = NULL;   // free(NULL) does nothing
	} else {
		data->len = ARRAYOBJECT_LEN(arr);
		if (!(data->val = malloc(data->len))) {
			errorobject_thrownomem(interp);
			free(data);
			return NULL;
		}

		for (size_t i=0; i < data->len; i++)
		{
			struct Object *integer = ARRAYOBJECT_GET(arr, i);
			if (!check_type(interp, interp->builtins.Integer, integer)) {
				free(data->val);
				free(data);
				return NULL;
			}

			long long val = integerobject_tolonglong(integer);
			if (val < 0x00 || val > 0xff) {
				errorobject_throwfmt(interp, "ValueError", "expected an integer between 0 and 255, got %D", integer);
				free(data->val);
				free(data);
				return NULL;
			}
			data->val[i] = val;
		}
	}

	struct Object *b = object_new_noerr(interp, klass, (struct ObjectData){.data=data, .foreachref=NULL, .destructor=bytearray_destructor});
	if (!b) {
		free(data->val);
		free(data);
		return NULL;
	}
	return b;
}

// overrides Object's setup to allow arguments
static bool setup(struct Interpreter *interp, struct ObjectData nulldata, struct Object *args, struct Object *opts) {
	return true;
}

static bool validate_index(struct Interpreter *interp, size_t len, long long i)
{
	if (i < 0) {
		errorobject_throwfmt(interp, "ValueError", "%L is not a valid array index", i);
		return false;
	}
	if ((unsigned long long) i >= len) {
		errorobject_throwfmt(interp, "ValueError", "%L is not a valid index for an array of length %L", i, (long long)len);
		return false;
	}
	return true;
}

static struct Object *get(struct Interpreter *interp, struct ObjectData thisdata, struct Object *args, struct Object *opts)
{
	if (!check_args(interp, args, interp->builtins.Integer, NULL)) return NULL;
	if (!check_no_opts(interp, opts)) return NULL;
	struct Object *b = thisdata.data;
	struct Object *iobj = ARRAYOBJECT_GET(args, 0);

	long long i = integerobject_tolonglong(iobj);
	if (!validate_index(interp, BYTEARRAYOBJECT_LEN(b), i))
		return NULL;
	return integerobject_newfromlonglong(interp, BYTEARRAYOBJECT_DATA(b)[i]);
}

static struct Object *slice(struct Interpreter *interp, struct ObjectData thisdata, struct Object *args, struct Object *opts)
{
	struct Object *b = thisdata.data;

	if (!check_no_opts(interp, opts))
		return NULL;
	long long start, end;
	if (ARRAYOBJECT_LEN(args) == 1) {
		// (thing.slice i) is same as (thing.slice i thing.length)
		if (!check_args(interp, args, interp->builtins.Integer, NULL))
			return NULL;
		start = integerobject_tolonglong(ARRAYOBJECT_GET(args, 0));
		end = BYTEARRAYOBJECT_LEN(b);
	} else {
		if (!check_args(interp, args, interp->builtins.Integer, interp->builtins.Integer, NULL))
			return NULL;
		start = integerobject_tolonglong(ARRAYOBJECT_GET(args, 0));
		end = integerobject_tolonglong(ARRAYOBJECT_GET(args, 1));
	}

	// ignore out of bounds indexes, like in python
	if (start < 0)
		start = 0;
	if (end < 0)
		return bytearrayobject_new(interp, NULL, 0);
	// now end is guaranteed to be non-negative, so it can be casted to size_t
	if ((size_t)end > BYTEARRAYOBJECT_LEN(b))
		end = BYTEARRAYOBJECT_LEN(b);
	if (start >= end)
		return bytearrayobject_new(interp, NULL, 0);

	// TODO: optimize magically instead of allocating more mem for each slice
	// maybe a ByteArraySlice object like python's memoryview?
	unsigned char *newdata = malloc(end - start);
	if (!newdata) {
		errorobject_thrownomem(interp);
		return NULL;
	}
	memcpy(newdata, BYTEARRAYOBJECT_DATA(b)+start, end-start);
	return bytearrayobject_new(interp, newdata, end - start);
}

static struct Object *length_getter(struct Interpreter *interp, struct ObjectData nulldata, struct Object *args, struct Object *opts)
{
	if (!check_args(interp, args, interp->builtins.ByteArray, NULL)) return NULL;
	if (!check_no_opts(interp, opts)) return NULL;
	return integerobject_newfromlonglong(interp, BYTEARRAYOBJECT_LEN(ARRAYOBJECT_GET(args, 0)));
}

struct Object *bytearrayobject_createclass(struct Interpreter *interp)
{
	struct Object *klass = classobject_new(interp, "ByteArray", interp->builtins.Object, newinstance);
	if (!klass)
		return NULL;

	if (!method_add_noret(interp, klass, "setup", setup)) goto error;
	if (!attribute_add(interp, klass, "length", length_getter, NULL)) goto error;
	if (!method_add_yesret(interp, klass, "get", get)) goto error;
	if (!method_add_yesret(interp, klass, "slice", slice)) goto error;
	return klass;

error:
	OBJECT_DECREF(interp, klass);
	return NULL;
}


struct Object *bytearrayobject_new(struct Interpreter *interp, unsigned char *val, size_t len)
{
	struct ByteArrayObjectData *data = malloc(sizeof(struct ByteArrayObjectData));
	if (!data) {
		errorobject_thrownomem(interp);
		free(val);
		return NULL;
	}

	data->len = len;
	data->val = val;

	struct Object *b = object_new_noerr(interp, interp->builtins.ByteArray, (struct ObjectData){.data=data, .foreachref=NULL, .destructor=bytearray_destructor});
	if (!b) {
		errorobject_thrownomem(interp);
		free(data);
		free(val);
		return NULL;
	}
	b->hashable = false;    // FIXME
	return b;
}


#define BOOL_OPTION(interp, b) optionobject_new((interp), (b) ? (interp)->builtins.yes : (interp)->builtins.no)

static struct Object *eq(struct Interpreter *interp, struct ObjectData nulldata, struct Object *args, struct Object *opts)
{
	if (!check_args(interp, args, interp->builtins.Object, interp->builtins.Object, NULL)) return NULL;
	if (!check_no_opts(interp, opts)) return NULL;

	struct Object *b1 = ARRAYOBJECT_GET(args, 0), *b2 = ARRAYOBJECT_GET(args, 1);
	if (!(classobject_isinstanceof(b1, interp->builtins.ByteArray) && classobject_isinstanceof(b2, interp->builtins.ByteArray))) {
		OBJECT_INCREF(interp, interp->builtins.none);
		return interp->builtins.none;
	}

	if (b1 == b2)
		return BOOL_OPTION(interp, true);
	if (BYTEARRAYOBJECT_LEN(b1) != BYTEARRAYOBJECT_LEN(b2))
		return BOOL_OPTION(interp, false);
	return BOOL_OPTION(interp,
		memcmp(BYTEARRAYOBJECT_DATA(b1), BYTEARRAYOBJECT_DATA(b2), BYTEARRAYOBJECT_LEN(b1))==0);
}

bool bytearrayobject_initoparrays(struct Interpreter *interp)
{
	return functionobject_add2array(interp, interp->oparrays.eq, "bytearray_eq", functionobject_mkcfunc_yesret(eq));
}
