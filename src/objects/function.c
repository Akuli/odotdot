#include "function.h"
#include <assert.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include "../attribute.h"
#include "../check.h"
#include "../interpreter.h"
#include "../method.h"
#include "../objectsystem.h"
#include "array.h"
#include "classobject.h"
#include "errors.h"
#include "mapping.h"
#include "null.h"
#include "string.h"


struct FunctionData {
	struct Object *name;
	functionobject_cfunc cfunc;
	struct Object *partialargs;  // an Array
};

static void function_foreachref(struct Object *func, void *cbdata, object_foreachrefcb cb)
{
	struct FunctionData *data = func->data;    // casts implicitly
	cb(data->name, cbdata);
	cb(data->partialargs, cbdata);
}

static void function_destructor(struct Object *func)
{
	free(func->data);
}

static struct Object *newinstance(struct Interpreter *interp, struct Object *args, struct Object *opts)
{
	errorobject_throwfmt(interp, "TypeError", "cannot create new function objects like (new Function), use func or lambda instead");
	return NULL;
}

struct Object *functionobject_createclass(struct Interpreter *interp)
{
	return classobject_new(interp, "Function", interp->builtins.Object, newinstance);
}


static struct Object *create_a_partial(struct Interpreter *interp, struct Object *func, struct Object **partialargs, size_t npartialargs)
{
	if (npartialargs == 0)
		return func;

	struct FunctionData *data = func->data;
	struct FunctionData *newdata = malloc(sizeof(struct FunctionData));
	if (!newdata) {
		errorobject_thrownomem(interp);
		return NULL;
	}

	// TODO: take the new partialargs as an array and use arrayobject_concat?
	newdata->cfunc = data->cfunc;
	if (!(newdata->partialargs = arrayobject_newwithcapacity(interp, ARRAYOBJECT_LEN(data->partialargs) + npartialargs))) {
		free(newdata);
		return NULL;
	}

	for (size_t i=0; i < ARRAYOBJECT_LEN(data->partialargs); i++) {
		if (!arrayobject_push(interp, newdata->partialargs, ARRAYOBJECT_GET(data->partialargs, i))){
			OBJECT_DECREF(interp, newdata->partialargs);
			free(newdata);
			return NULL;
		}
	}
	for (size_t i=0; i < npartialargs; i++) {
		if (!arrayobject_push(interp, newdata->partialargs, partialargs[i])){
			OBJECT_DECREF(interp, newdata->partialargs);
			free(newdata);
			return NULL;
		}
	}

	// TODO: should this be changed? if it should, must check all uses of this
	newdata->name = data->name;
	OBJECT_INCREF(interp, newdata->name);

	struct Object *obj = object_new_noerr(interp, interp->builtins.Function, newdata, function_foreachref, function_destructor);
	if (!obj) {
		errorobject_thrownomem(interp);
		OBJECT_DECREF(interp, newdata->name);
		OBJECT_DECREF(interp, newdata->partialargs);
		free(newdata);
		return NULL;
	}
	return obj;
}

static struct Object *partial(struct Interpreter *interp, struct Object *args, struct Object *opts)
{
	if (!check_no_opts(interp, opts)) return NULL;

	// check the first argument, rest are the args that are being partialled
	// there can be 0 or more partialled args (0 partialled args allowed for consistency)
	if (ARRAYOBJECT_LEN(args) == 0) {
		errorobject_throwfmt(interp, "ArgError", "not enough arguments to Function.partial");
		return NULL;
	}
	if (!check_type(interp, interp->builtins.Function, ARRAYOBJECT_GET(args, 0)))
		return NULL;

	return create_a_partial(interp, ARRAYOBJECT_GET(args, 0),
		((struct ArrayObjectData *) args->data)->elems + 1, ARRAYOBJECT_LEN(args) - 1);
}

struct Object *functionobject_newpartial(struct Interpreter *interp, struct Object *func, struct Object *partialarg)
{
	return create_a_partial(interp, func, &partialarg, 1);
}


static struct Object *name_getter(struct Interpreter *interp, struct Object *args, struct Object *opts)
{
	if (!check_args(interp, args, interp->builtins.Function, NULL)) return NULL;
	if (!check_no_opts(interp, opts)) return NULL;

	struct FunctionData *data = ARRAYOBJECT_GET(args, 0)->data;
	OBJECT_INCREF(interp, data->name);
	return data->name;
}

static struct Object *name_setter(struct Interpreter *interp, struct Object *args, struct Object *opts)
{
	if (!check_args(interp, args, interp->builtins.Function, interp->builtins.String, NULL)) return NULL;
	if (!check_no_opts(interp, opts)) return NULL;

	struct FunctionData *data = ARRAYOBJECT_GET(args, 0)->data;
	OBJECT_DECREF(interp, data->name);
	data->name = ARRAYOBJECT_GET(args, 1);
	OBJECT_INCREF(interp, data->name);
	return nullobject_get(interp);
}

bool functionobject_setname(struct Interpreter *interp, struct Object *func, char *newname)
{
	struct Object *newnameobj = stringobject_newfromcharptr(interp, newname);
	if (!newnameobj)
		return false;

	struct FunctionData *data = func->data;
	OBJECT_DECREF(interp, data->name);
	data->name = newnameobj;
	// no need to incref, newnameobj is already holding a reference
	return true;
}


static struct Object *setup(struct Interpreter *interp, struct Object *args, struct Object *opts)
{
	// FIXME: ValueError feels wrong for this
	errorobject_throwfmt(interp, "ValueError", "functions can't be created with (new Function), use func instead");
	return NULL;
}


bool functionobject_addmethods(struct Interpreter *interp)
{
	if (!attribute_add(interp, interp->builtins.Function, "name", name_getter, name_setter)) return false;
	if (!method_add(interp, interp->builtins.Function, "setup", setup)) return false;
	if (!method_add(interp, interp->builtins.Function, "partial", partial)) return false;
	return true;
}


struct Object *functionobject_new(struct Interpreter *interp, functionobject_cfunc cfunc, char *name)
{
	struct FunctionData *data = malloc(sizeof(struct FunctionData));
	if (!data) {
		errorobject_thrownomem(interp);
		return NULL;
	}

	struct Object *nameobj = stringobject_newfromcharptr(interp, name);
	if (!nameobj) {
		free(data);
		return NULL;
	}

	data->name = nameobj;
	data->cfunc = cfunc;
	if (!(data->partialargs = arrayobject_newempty(interp))) {
		OBJECT_DECREF(interp, nameobj);
		free(data);
		return NULL;
	}

	struct Object *obj = object_new_noerr(interp, interp->builtins.Function, data, function_foreachref, function_destructor);
	if (!obj) {
		errorobject_thrownomem(interp);
		OBJECT_DECREF(interp, data->partialargs);
		OBJECT_DECREF(interp, nameobj);
		free(data);
		return NULL;
	}

	return obj;
}

bool functionobject_add2array(struct Interpreter *interp, struct Object *arr, char *name, functionobject_cfunc cfunc)
{
	struct Object *func = functionobject_new(interp, cfunc, name);
	if (!func)
		return false;

	bool ok = arrayobject_push(interp, arr, func);
	OBJECT_DECREF(interp, func);
	return ok;
}


struct Object *functionobject_call(struct Interpreter *interp, struct Object *func, ...)
{
	struct Object *args = arrayobject_newempty(interp);
	if (!args)
		return NULL;

	va_list ap;
	va_start(ap, func);

	while(true) {
		struct Object *arg = va_arg(ap, struct Object *);
		if (!arg)    // end of argument list, not an error
			break;
		if (!arrayobject_push(interp, args, arg)) {
			OBJECT_DECREF(interp, args);
			return NULL;
		}
	}
	va_end(ap);

	struct Object *opts = mappingobject_newempty(interp);
	if (!opts)
		return NULL;

	struct Object *res = functionobject_vcall(interp, func, args, opts);
	OBJECT_DECREF(interp, opts);
	OBJECT_DECREF(interp, args);
	return res;
}

struct Object *functionobject_vcall(struct Interpreter *interp, struct Object *func, struct Object *args, struct Object *opts)
{
	struct FunctionData *data = func->data;     // casts implicitly

	struct Object *theargs;
	bool decreftheargs = false;
	if (ARRAYOBJECT_LEN(data->partialargs) == 0) {
		theargs = args;
	} else if (ARRAYOBJECT_LEN(args) == 0) {
		theargs = data->partialargs;
	} else {
		theargs = arrayobject_concat(interp, data->partialargs, args);
		if (!theargs)
			return NULL;
		decreftheargs = true;
	}

	struct Object *res = data->cfunc(interp, theargs, opts);
	if (decreftheargs)
		OBJECT_DECREF(interp, theargs);
	return res;   // may be NULL
}
