#include <assert.h>
#include "function.h"
#include <stdlib.h>
#include "../common.h"
#include "../interpreter.h"
#include "../objectsystem.h"
#include "classobject.h"

// because void* can't hold function pointers according to the standard
struct FunctionData {
	functionobject_cfunc cfunc;
};

static void function_destructor(struct Object *funcobj)
{
	free(funcobj->data);
}

int functionobject_createclass(struct Interpreter *interp, struct Object **errptr)
{
	struct Object *objectclass = interpreter_getbuiltin(interp, errptr, "Object");
	if (!objectclass)    // errptr is set already
		return STATUS_ERROR;

	struct Object *klass = classobject_new(interp, errptr, "Function", objectclass, NULL, function_destructor);
	OBJECT_DECREF(interp, objectclass);
	if (!klass)
		return STATUS_ERROR;

	interp->functionclass = klass;
	return STATUS_OK;
}

struct Object *functionobject_new(struct Interpreter *interp, struct Object **errptr, functionobject_cfunc cfunc)
{
	struct FunctionData *data = malloc(sizeof(struct FunctionData));
	if (!data) {
		*errptr = interp->nomemerr;
		return NULL;
	}
	data->cfunc = cfunc;

	assert(interp->functionclass);
	struct Object *obj = classobject_newinstance(interp, errptr, interp->functionclass, data);
	if (!obj) {
		free(data);
		return NULL;
	}
	return obj;
}

functionobject_cfunc functionobject_getcfunc(struct Interpreter *interp, struct Object *func)
{
	// TODO: better type check using errptr
	assert(func->klass == interp->functionclass);

	return ((struct FunctionData *) func->data)->cfunc;
}
