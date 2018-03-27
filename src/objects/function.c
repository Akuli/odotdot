#include "function.h"
#include <stdlib.h>
#include "../common.h"
#include "../objectsystem.h"

struct FunctionData {
	functionobject_cfunc cfunc;
};

static int function_destructor(struct Object *funcobj)
{
	free(funcobj->data);
	return STATUS_OK;
}

struct ObjectClassInfo *functionobject_createclass(struct ObjectClassInfo *objectclass)
{
	return objectclassinfo_new(objectclass, function_destructor);
}

struct Object *functionobject_new(struct ObjectClassInfo *functionclass, functionobject_cfunc cfunc)
{
	struct FunctionData *data = malloc(sizeof(struct FunctionData));
	if (!data)
		return NULL;
	data->cfunc = cfunc;

	struct Object *obj = object_new(functionclass);
	if (!obj) {
		free(data);
		return NULL;
	}
	obj->data = data;
	return obj;
}

// TODO: better API for calling the functions
functionobject_cfunc functionobject_get_cfunc(struct Object *funcobj)
{
	return ((struct FunctionData*) funcobj->data)->cfunc;
}
