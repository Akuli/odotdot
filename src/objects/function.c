#include "function.h"
#include <assert.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include "classobject.h"
#include "errors.h"
#include "mapping.h"
#include "../common.h"
#include "../interpreter.h"
#include "../method.h"
#include "../objectsystem.h"

/* see below for how this is used
   passing more than 10 arguments would be kinda insane, and more than 20 would be really insane
   if this was a part of some kind of API, i would allow at least 100 arguments though
   but calling a function from ö doesn't call anything that uses this, so that's no problem */
#define NARGS_MAX 20


struct FunctionData {
	functionobject_cfunc cfunc;
	struct Object **partialargs;
	size_t npartialargs;
};

static void function_foreachref(struct Object *func, void *cbdata, classobject_foreachrefcb cb)
{
	struct FunctionData *data = func->data;    // casts implicitly
	for (size_t i=0; i < data->npartialargs; i++)
		cb(data->partialargs[i], cbdata);
}

static void function_destructor(struct Object *func)
{
	// object_free_impl() takes care of partial args using function_foreachref()
	struct FunctionData *data = func->data;
	if (data->partialargs)
		free(data->partialargs);
	free(data);
}

struct Object *functionobject_createclass(struct Interpreter *interp, struct Object **errptr)
{
	return classobject_new(interp, errptr, "Function", interp->builtins.objectclass, 0, function_foreachref);
}

static struct Object *partial(struct Interpreter *interp, struct Object **errptr, struct Object **args, size_t nargs)
{
	// check the first argument, rest are the args that are being partialled
	// there can be 0 or more partialled args (0 partialled args allowed for consistency)
	if (functionobject_checktypes(interp, errptr, args, (nargs>1 ? 1 : nargs), interp->builtins.functionclass, NULL) == STATUS_ERROR)
		return NULL;
	struct Object *func = args[0];
	struct Object **partialargs = args + 1;
	size_t npartialargs = nargs - 1;

	// shortcut: no partial args to add
	if (nargs == 1) {
		OBJECT_INCREF(interp, func);
		return func;
	}

	struct FunctionData *data = func->data;     // casts implicitly
	struct FunctionData *newdata = malloc(sizeof(struct FunctionData));
	if (!newdata) {
		errorobject_setnomem(interp, errptr);
		return NULL;
	}

	newdata->cfunc = data->cfunc;
	newdata->npartialargs = data->npartialargs + npartialargs;
	newdata->partialargs = malloc(sizeof(struct Object*) * newdata->npartialargs);
	if (!newdata->partialargs) {
		errorobject_setnomem(interp, errptr);
		free(newdata);
		return NULL;
	}

	memcpy(newdata->partialargs, partialargs, sizeof(struct Object*) * npartialargs);
	memcpy(newdata->partialargs + npartialargs, data->partialargs, sizeof(struct Object*) * data->npartialargs);
	for (size_t i=0; i < newdata->npartialargs; i++)
		OBJECT_INCREF(interp, newdata->partialargs[i]);

	struct Object *obj = classobject_newinstance(interp, errptr, interp->builtins.functionclass, newdata, function_destructor);
	if (!obj) {
		for (size_t i=0; i < newdata->npartialargs; i++)
			OBJECT_DECREF(interp, newdata->partialargs[i]);
		free(newdata->partialargs);
		free(newdata);
		return NULL;
	}
	return obj;
}

// methods are partialled functions, but the partial method can't be used when looking up methods
// that would be infinite recursion
struct Object *functionobject_newpartial(struct Interpreter *interp, struct Object **errptr, struct Object *func, struct Object *partialarg)
{
	struct Object *args[] = { func, partialarg };
	return partial(interp, errptr, args, 2);
}

int functionobject_addmethods(struct Interpreter *interp, struct Object **errptr)
{
	// TODO: map, to_string
	if (method_add(interp, errptr, interp->builtins.functionclass, "partial", partial) == STATUS_ERROR) return STATUS_ERROR;
	return STATUS_OK;
}

int functionobject_checktypes(struct Interpreter *interp, struct Object **errptr, struct Object **args, size_t nargs, ...)
{
	va_list ap;
	va_start(ap, nargs);

	unsigned int expectnargs;   // expected number of arguments
	struct Object *classes[NARGS_MAX];
	for (expectnargs=0; expectnargs < NARGS_MAX; expectnargs++) {
		struct Object *klass = va_arg(ap, struct Object *);
		if (!klass)
			break;      // end of argument list, not an error
		classes[expectnargs] = klass;
	}
	va_end(ap);

	// TODO: test these
	// TODO: include the function name in the error?
	if (nargs != expectnargs) {
		errorobject_setwithfmt(interp, errptr, "%s arguments", nargs > expectnargs ? "too many" : "not enough");
		for (unsigned int i=0; i < expectnargs; i++)
			OBJECT_DECREF(interp, classes[i]);
		return STATUS_ERROR;
	}

	for (unsigned int i=0; i < nargs; i++) {
		if (errorobject_typecheck(interp, errptr, classes[i], args[i]) == STATUS_ERROR)
			return STATUS_ERROR;
	}

	return STATUS_OK;
}

struct Object *functionobject_new(struct Interpreter *interp, struct Object **errptr, functionobject_cfunc cfunc)
{
	struct FunctionData *data = malloc(sizeof(struct FunctionData));
	if (!data) {
		errorobject_setnomem(interp, errptr);
		return NULL;
	}
	data->cfunc = cfunc;
	data->partialargs = NULL;
	data->npartialargs = 0;

	struct Object *obj = classobject_newinstance(interp, errptr, interp->builtins.functionclass, data, function_destructor);
	if (!obj) {
		free(data);
		return NULL;
	}
	return obj;
}

struct Object *functionobject_call(struct Interpreter *interp, struct Object **errptr, struct Object *func, ...)
{
	assert(func->klass == interp->builtins.functionclass);    // TODO: better type check

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

	return functionobject_vcall(interp, errptr, func, args, nargs);
}

struct Object *functionobject_vcall(struct Interpreter *interp, struct Object **errptr, struct Object *func, struct Object **args, size_t nargs)
{
	struct Object **theargs;
	size_t thenargs;
	struct FunctionData *data = func->data;     // casts implicitly

	if (data->npartialargs > 0) {
		// TODO: is there a more efficient way to do this?
		thenargs = nargs + data->npartialargs;
		theargs = malloc(sizeof(struct Object *) * thenargs);
		if (!theargs) {
			errorobject_setnomem(interp, errptr);
			return NULL;
		}
		memcpy(theargs, data->partialargs, sizeof(struct Object*) * data->npartialargs);
		memcpy(theargs + data->npartialargs, args, sizeof(struct Object*) * nargs);
	} else {
		thenargs = nargs;
		theargs = args;
	}

	struct Object *res = data->cfunc(interp, errptr, theargs, thenargs);
	if (data->npartialargs > 0)
		free(theargs);
	return res;
}
