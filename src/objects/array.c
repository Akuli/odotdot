#include "array.h"
#include <assert.h>
#include "classobject.h"
#include "../common.h"
#include "../dynamicarray.h"
#include "../interpreter.h"
#include "../objectsystem.h"

static void array_foreachref(struct Object *obj, void *data, objectclassinfo_foreachrefcb cb)
{
	struct DynamicArray *dynarray = obj->data;
	for (size_t i=0; i < dynarray->len; i++)
		cb(dynarray->values[i], data);
}

static void array_destructor(struct Object *arr)
{
	dynamicarray_free(arr->data);
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

	int res = interpreter_addbuiltin(interp, errptr, "Array", klass);
	OBJECT_DECREF(interp, klass);
	return res;
}

// TODO: add something for creating DynamicArrays from existing item lists efficiently
struct Object *arrayobject_newempty(struct Interpreter *interp, struct Object **errptr)
{
	struct DynamicArray *dynarray = dynamicarray_new();
	if (!dynarray)
		return NULL;

	struct Object *klass = interpreter_getbuiltin(interp, errptr, "Array");
	if (!klass) {
		dynamicarray_free(dynarray);
		return NULL;
	}

	struct Object *arr = classobject_newinstance(interp, errptr, klass, dynarray);
	OBJECT_DECREF(interp, klass);
	if (!arr) {
		dynamicarray_free(dynarray);
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
			OBJECT_DECREF(interp, arr);
			return NULL;
		}
	}

	// this way stuff doesn't need to be decreffed in a second loop when dynamicarray_push fails
	for (size_t i=0; i < nelems; i++)
		OBJECT_INCREF(interp, elems[i]);

	return arr;
}
