#include "function.h"
#include <stdlib.h>
#include "../objectsystem.h"

// because function pointers can't be void* pointers according to the standard
struct FunctionData {
	functionobject_cfunc cfunc;
};

static void function_destructor(struct Object *funcobj)
{
	free(funcobj->data);
}

struct ObjectClassInfo *functionobject_createclass(struct ObjectClassInfo *objectclass)
{
	return objectclassinfo_new(objectclass, NULL, function_destructor);
}

struct Object *functionobject_new(struct ObjectClassInfo *functionclass, functionobject_cfunc cfunc)
{
	struct FunctionData *data = malloc(sizeof(struct FunctionData));
	if (!data)
		return NULL;
	data->cfunc = cfunc;

	struct Object *obj = object_new(functionclass, data);
	if (!obj) {
		free(data);
		return NULL;
	}
	return obj;
}

// TODO: better API for calling the functions
functionobject_cfunc functionobject_get_cfunc(struct Object *funcobj)
{
	return ((struct FunctionData*) funcobj->data)->cfunc;
}
