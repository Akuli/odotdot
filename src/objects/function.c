#include <assert.h>
#include "function.h"
#include <stdlib.h>
#include "../common.h"
#include "../interpreter.h"
#include "../objectsystem.h"

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

	struct ObjectClassInfo *klass = objectclassinfo_new(objectclass->data, NULL, function_destructor);
	if (!klass) {
		*errptr = interp->nomemerr;
		// TODO: decref objectclass??
		return STATUS_ERROR;
	}

	interp->functionobjectinfo = klass;
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

	assert(interp->functionobjectinfo);
	struct Object *obj = object_new(interp->functionobjectinfo, data);
	if (!obj) {
		*errptr = interp->nomemerr;
		free(data);
		return NULL;
	}
	return obj;
}

functionobject_cfunc functionobject_getcfunc(struct Interpreter *interp, struct Object **errptr, struct Object *func)
{
	// TODO: better type check using errptr
	assert(func->klass == interp->functionobjectinfo);

	return ((struct FunctionData *) func->data)->cfunc;
}
