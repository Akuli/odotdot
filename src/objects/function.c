#include "function.h"
#include <assert.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include "../common.h"
#include "../interpreter.h"
#include "../objectsystem.h"
#include "classobject.h"

/* see below for how this is used
   passing more than 10 arguments would be kinda insane, and more than 20 would be really insane
   if this was a part of some kind of API, i would allow at least 100 arguments though
   but calling a function from ö doesn't call anything that uses this, so that's no problem */
#define NARGS_MAX 20


struct FunctionData {
	functionobject_cfunc cfunc;
	struct Object *partialarg;
	struct Object *extraref;
};

static void function_foreachref(struct Object *func, void *cbdata, objectclassinfo_foreachrefcb cb)
{
	struct FunctionData *data = func->data;    // casts implicitly
	if (data->partialarg)
		cb(data->partialarg, cbdata);
	if (data->extraref)
		cb(data->extraref, cbdata);
}

static void function_destructor(struct Object *func)
{
	// object_free_impl() takes care of partialarg using function_foreachref()
	free(func->data);
}

int functionobject_createclass(struct Interpreter *interp, struct Object **errptr)
{
	struct Object *objectclass = interpreter_getbuiltin(interp, errptr, "Object");
	if (!objectclass)    // errptr is set already
		return STATUS_ERROR;

	struct Object *klass = classobject_new(interp, errptr, "Function", objectclass, function_foreachref, function_destructor);
	OBJECT_DECREF(interp, objectclass);
	if (!klass)
		return STATUS_ERROR;

	interp->functionclass = klass;
	return STATUS_OK;
}

int functionobject_checktypes(struct Context *ctx, struct Object **errptr, struct Object **args, size_t nargs, ...)
{
	va_list ap;
	va_start(ap, nargs);

	int expectnargs;   // expected number of arguments
	struct Object *classes[NARGS_MAX];
	for (expectnargs=0; expectnargs < NARGS_MAX; expectnargs++) {
		char *classname = va_arg(ap, char*);
		if (!classname)
			break;

		struct Object *klass = interpreter_getbuiltin(ctx->interp, errptr, classname);
		if (!klass) {
			for (int i=0; i < expectnargs; i++)
				OBJECT_DECREF(ctx->interp, classes[i]);
			return STATUS_ERROR;
		}
		assert(klass->klass == ctx->interp->classclass);
		classes[expectnargs] = klass;
	}
	va_end(ap);

	// TODO: do actual errors instead of stupid assertions :(
	assert(nargs == (size_t) expectnargs);
	for (int i=0; i < expectnargs; i++) {
		assert(classobject_istypeof(classes[i], args[i]));
		OBJECT_DECREF(ctx->interp, classes[i]);
	}

	return STATUS_OK;
}

struct Object *functionobject_new(struct Interpreter *interp, struct Object **errptr, functionobject_cfunc cfunc, struct Object *partialarg)
{
	struct FunctionData *data = malloc(sizeof(struct FunctionData));
	if (!data) {
		*errptr = interp->nomemerr;
		return NULL;
	}
	data->cfunc = cfunc;
	data->partialarg = partialarg;
	if (partialarg)
		OBJECT_INCREF(interp, partialarg);
	data->extraref = NULL;

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

struct Object *functionobject_call(struct Context *ctx, struct Object **errptr, struct Object *func, ...)
{
	assert(func->klass == ctx->interp->functionclass);    // TODO: better type check

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
	assert(func->klass == ctx->interp->functionclass);    // TODO: better type check
	struct Object **theargs;
	size_t thenargs;
	struct FunctionData *data = func->data;     // casts implicitly

	if (data->partialarg) {
		// FIXME: this sucks
		thenargs = nargs+1;
		theargs = malloc(sizeof(struct Object *) * thenargs);
		if (!theargs) {
			*errptr = ctx->interp->nomemerr;
			return NULL;
		}
		theargs[0] = data->partialarg;
		memcpy(theargs+1, args, sizeof(struct Object *) * nargs);
	} else {
		thenargs = nargs;
		theargs = args;
	}

	struct Object *res = data->cfunc(ctx, errptr, theargs, thenargs);
	if (data->partialarg)
		free(theargs);
	return res;
}

struct Object *functionobject_newpartial(struct Interpreter *interp, struct Object **errptr, struct Object *func, struct Object *partialarg)
{
	assert(func->klass == interp->functionclass);    // TODO: better type check
	struct FunctionData *data = func->data;
	assert(data->partialarg == NULL);       // FIXME

	struct Object *partialled = functionobject_new(interp, errptr, data->cfunc, partialarg);
	((struct FunctionData *) partialled->data)->extraref = func;
	OBJECT_INCREF(interp, func);
	return partialled;
}
