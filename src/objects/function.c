#include <assert.h>
#include "function.h"
#include <stdarg.h>
#include <stdlib.h>
#include "../common.h"
#include "../interpreter.h"
#include "../objectsystem.h"
#include "classobject.h"

// because void* can't hold function pointers according to the standard
struct FunctionData {
	functionobject_cfunc cfunc;
	void *data;
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

struct Object *functionobject_new(struct Interpreter *interp, struct Object **errptr, functionobject_cfunc cfunc, void *data)
{
	struct FunctionData *funcdata = malloc(sizeof(struct FunctionData));
	if (!funcdata) {
		*errptr = interp->nomemerr;
		return NULL;
	}
	funcdata->cfunc = cfunc;
	funcdata->data = data;

	assert(interp->functionclass);
	struct Object *obj = classobject_newinstance(interp, errptr, interp->functionclass, funcdata);
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

// passing more than 10 arguments would be kinda insane, and more than 20 would be really insane
// if this was a part of some kind of API, i would allow at least 100 arguments though
// but calling a function from ö doesn't call this, so that's no problem
#define NARGS_MAX 20

struct Object *functionobject_call(struct Context *ctx, struct Object **errptr, struct Object *func, ...)
{
	assert(func->klass == ctx->interp->functionclass);

	struct Object *args[NARGS_MAX];
	va_list ap;
	va_start(ap, func);
	int nargs;
	for (nargs=0; nargs < NARGS_MAX; nargs++) {
		struct Object *arg = va_arg(ap, struct Object *);
		if (!arg)
			break;
		args[nargs] = arg;
	}
	va_end(ap);

	return functionobject_vcall(ctx, errptr, func, args, nargs);
}

struct Object *functionobject_vcall(struct Context *ctx, struct Object **errptr, struct Object *func, struct Object **args, size_t nargs)
{
	struct FunctionData *funcdata = func->data;     // casts implicitly
	return funcdata->cfunc(ctx, errptr, args, nargs, funcdata->data);
}

#undef NARGS_MAX
