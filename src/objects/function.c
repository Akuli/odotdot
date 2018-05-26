#include "function.h"
#include <assert.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include "../check.h"
#include "../common.h"
#include "../interpreter.h"
#include "../method.h"
#include "../objectsystem.h"
#include "array.h"
#include "classobject.h"
#include "errors.h"
#include "mapping.h"

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

struct Object *functionobject_createclass(struct Interpreter *interp)
{
	return classobject_new(interp, "Function", interp->builtins.objectclass, 0, function_foreachref);
}

static struct Object *partial(struct Interpreter *interp, struct Object *argarr)
{
	// check the first argument, rest are the args that are being partialled
	// there can be 0 or more partialled args (0 partialled args allowed for consistency)
	if (ARRAYOBJECT_LEN(argarr) == 0) {
		errorobject_setwithfmt(interp, "not enough arguments to Function::partial");
		return NULL;
	}
	if (check_type(interp, interp->builtins.functionclass, ARRAYOBJECT_GET(argarr, 0)) == STATUS_ERROR)
		return NULL;

	struct Object *func = ARRAYOBJECT_GET(argarr, 0);
	struct Object **partialargs = ((struct ArrayObjectData *) argarr->data)->elems + 1;
	size_t npartialargs = ARRAYOBJECT_LEN(argarr) - 1;

	// shortcut
	if (npartialargs == 0) {
		OBJECT_INCREF(interp, func);
		return func;
	}

	struct FunctionData *data = func->data;     // casts implicitly
	struct FunctionData *newdata = malloc(sizeof(struct FunctionData));
	if (!newdata) {
		errorobject_setnomem(interp);
		return NULL;
	}

	newdata->cfunc = data->cfunc;
	newdata->npartialargs = data->npartialargs + npartialargs;
	newdata->partialargs = malloc(sizeof(struct Object*) * newdata->npartialargs);
	if (!newdata->partialargs) {
		errorobject_setnomem(interp);
		free(newdata);
		return NULL;
	}

	memcpy(newdata->partialargs, partialargs, sizeof(struct Object*) * npartialargs);
	memcpy(newdata->partialargs + npartialargs, data->partialargs, sizeof(struct Object*) * data->npartialargs);
	for (size_t i=0; i < newdata->npartialargs; i++)
		OBJECT_INCREF(interp, newdata->partialargs[i]);

	struct Object *obj = classobject_newinstance(interp, interp->builtins.functionclass, newdata, function_destructor);
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
struct Object *functionobject_newpartial(struct Interpreter *interp, struct Object *func, struct Object *partialarg)
{
	struct Object *tmp[] = { func, partialarg };
	struct Object *arr = arrayobject_new(interp, tmp, 2);
	if (!arr)
		return NULL;

	struct Object *res = partial(interp, arr);
	OBJECT_DECREF(interp, arr);
	return res;
}

int functionobject_addmethods(struct Interpreter *interp)
{
	// TODO: map, to_string
	if (method_add(interp, interp->builtins.functionclass, "partial", partial) == STATUS_ERROR) return STATUS_ERROR;
	return STATUS_OK;
}

struct Object *functionobject_new(struct Interpreter *interp, functionobject_cfunc cfunc)
{
	struct FunctionData *data = malloc(sizeof(struct FunctionData));
	if (!data) {
		errorobject_setnomem(interp);
		return NULL;
	}
	data->cfunc = cfunc;
	data->partialargs = NULL;
	data->npartialargs = 0;

	struct Object *obj = classobject_newinstance(interp, interp->builtins.functionclass, data, function_destructor);
	if (!obj) {
		free(data);
		return NULL;
	}
	return obj;
}

struct Object *functionobject_call(struct Interpreter *interp, struct Object *func, ...)
{
	assert(func->klass == interp->builtins.functionclass);    // TODO: better type check

	struct Object *argarr = arrayobject_newempty(interp);
	if (!argarr)
		return NULL;

	va_list ap;
	va_start(ap, func);

	while(true) {
		struct Object *arg = va_arg(ap, struct Object *);
		if (!arg)    // end of argument list, not an error
			break;
		if (arrayobject_push(interp, argarr, arg) == STATUS_ERROR) {
			OBJECT_DECREF(interp, argarr);
			return NULL;
		}
	}
	va_end(ap);

	struct Object *res = functionobject_vcall(interp, func, argarr);
	OBJECT_DECREF(interp, argarr);
	return res;
}

struct Object *functionobject_vcall(struct Interpreter *interp, struct Object *func, struct Object *argarr)
{
	struct Object *theargs;
	struct FunctionData *data = func->data;     // casts implicitly

	if (data->npartialargs > 0) {
		// TODO: add a more efficient way to concatenate arrays
		// TODO: data->partialargs should be an array
		theargs = arrayobject_newempty(interp);
		if (!theargs)
			return NULL;

		for (size_t i=0; i < data->npartialargs; i++) {
			if (arrayobject_push(interp, theargs, data->partialargs[i]) == STATUS_ERROR) {
				OBJECT_DECREF(interp, theargs);
				return NULL;
			}
		}
		for (size_t i=0; i < ARRAYOBJECT_LEN(argarr); i++) {
			if (arrayobject_push(interp, theargs, ARRAYOBJECT_GET(argarr, i)) == STATUS_ERROR) {
				OBJECT_DECREF(interp, theargs);
				return NULL;
			}
		}
	} else {
		theargs = argarr;
	}

	struct Object *res = data->cfunc(interp, theargs);
	if (theargs != argarr)
		OBJECT_DECREF(interp, theargs);
	return res;   // may be NULL
}
