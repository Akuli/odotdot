#include <assert.h>
#include <stdlib.h>
#include "array.h"
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

struct ObjectClassInfo *arrayobject_createclass(struct ObjectClassInfo *objectclass)
{
	return objectclassinfo_new(objectclass, array_foreachref, array_destructor);
}

struct Object *arrayobject_newempty(struct ObjectClassInfo *arrayclass)
{
	struct Object *arr = object_new(arrayclass);
	if(!arr)
		return NULL;

	arr->data = dynamicarray_new();
	if(!arr->data) {
		free(arr);
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
