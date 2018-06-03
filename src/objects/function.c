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
	return classobject_new(interp, "Function", interp->builtins.objectclass, function_foreachref);
}


static struct Object *setup(struct Interpreter *interp, struct Object *argarr)
{
	errorobject_setwithfmt(interp, "functions can't be created with (new Function), use func instead");
	return NULL;
}

static struct Object *create_a_partial(struct Interpreter *interp, struct Object *func, struct Object **partialargs, size_t npartialargs)
{
	// shortcut
	if (npartialargs == 0)
		return func;

	struct FunctionData *data = func->data;
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

	memcpy(newdata->partialargs, data->partialargs, sizeof(struct Object*) * data->npartialargs);
	memcpy(newdata->partialargs + data->npartialargs, partialargs, sizeof(struct Object*) * npartialargs);
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

static struct Object *partial(struct Interpreter *interp, struct Object *argarr)
{
	// check the first argument, rest are the args that are being partialled
	// there can be 0 or more partialled args (0 partialled args allowed for consistency)
	if (ARRAYOBJECT_LEN(argarr) == 0) {
		errorobject_setwithfmt(interp, "not enough arguments to Function.partial");
		return NULL;
	}
	if (check_type(interp, interp->builtins.functionclass, ARRAYOBJECT_GET(argarr, 0)) == STATUS_ERROR)
		return NULL;

	return create_a_partial(interp, ARRAYOBJECT_GET(argarr, 0),
		((struct ArrayObjectData *) argarr->data)->elems + 1, ARRAYOBJECT_LEN(argarr) - 1);
}

struct Object *functionobject_newpartial(struct Interpreter *interp, struct Object *func, struct Object *partialarg)
{
	return create_a_partial(interp, func, &partialarg, 1);
}

int functionobject_addmethods(struct Interpreter *interp)
{
	// TODO: to_string, map? map might as well be an array method or something
	if (method_add(interp, interp->builtins.functionclass, "setup", setup) == STATUS_ERROR) return STATUS_ERROR;
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
