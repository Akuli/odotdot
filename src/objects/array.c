#include "array.h"
#include <assert.h>
#include <stdlib.h>
#include "classobject.h"
#include "../common.h"
#include "../dynamicarray.h"
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

	struct Object *klass = classobject_new(interp, errptr, objectclass, array_foreachref, array_destructor);
	if (!klass) {
		// TODO: decref objectclass?
		*errptr = interp->nomemerr;
		return STATUS_ERROR;
	}

	// TODO: decref objectclass and klass if this returns STATUS_ERROR
	return interpreter_addbuiltin(interp, errptr, "Array", klass);
}

struct Object *arrayobject_newempty(struct ObjectClassInfo *arrayclass)
{
	struct DynamicArray *dynarray = dynamicarray_new();
	if (!dynarray)
		return NULL;

	struct Object *arr = object_new(arrayclass, dynarray);
	if (!arr) {
		dynamicarray_free(dynarray);
		return NULL;
	}
	return arr;
}

// TODO: reduce copy/pasting?
struct Object *arrayobject_newfromarrayandsize(struct ObjectClassInfo *arrayclass, struct Object **array, size_t size)
{
	struct Object *arr = arrayobject_newempty(arrayclass);
	if (!arr)
		return NULL;

	for (size_t i=0; i < size; i++) {
		int status = dynamicarray_push(arr->data, array[i]);
		if (status != STATUS_OK) {
			assert(status == STATUS_NOMEM);
			object_free(arr);
			return NULL;
		}
	}

	return arr;
}
struct Object *arrayobject_newfromnullterminated(struct ObjectClassInfo *arrayclass, struct Object **nullterminated)
{
	struct Object *arr = arrayobject_newempty(arrayclass);
	if (!arr)
		return NULL;

	for (size_t i=0; nullterminated[i]; i++) {
		int status = dynamicarray_push(arr->data, nullterminated[i]);
		if (status != STATUS_OK) {
			assert(status == STATUS_NOMEM);
			object_free(arr);
			return NULL;
		}
	}

	return arr;
}
