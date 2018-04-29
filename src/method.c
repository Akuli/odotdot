#include "method.h"
#include <assert.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include "common.h"
#include "interpreter.h"
#include "objects/classobject.h"
#include "objects/errors.h"
#include "context.h"
#include "hashtable.h"
#include "objects/function.h"
#include "objectsystem.h"
#include "unicode.h"

int method_add(struct Interpreter *interp, struct Object **errptr, struct Object *klass, char *name, functionobject_cfunc cfunc)
{
	// TODO: this should take ctx and use errorobject_typecheck()
	assert(klass->klass == interp->classclass);    // TODO: better type check

	struct UnicodeString *uname = malloc(sizeof(struct UnicodeString));
	if (!uname) {
		errorobject_setnomem(interp, errptr);
		return STATUS_ERROR;
	}

	char errormsg[100];
	int status = utf8_decode(name, strlen(name), uname, errormsg);
	if (status != STATUS_OK) {
		assert(status == STATUS_NOMEM);
		free(uname);
		errorobject_setnomem(interp, errptr);
		return STATUS_ERROR;
	}

	struct Object *func = functionobject_new(interp, errptr, cfunc);
	if (!func) {
		free(uname->val);
		free(uname);
		return STATUS_ERROR;
	}

	if (hashtable_set(((struct ObjectClassInfo *) klass->data)->methods, uname, unicodestring_hash(*uname), func, NULL) == STATUS_NOMEM) {
		free(uname->val);
		free(uname);
		OBJECT_DECREF(interp, func);
		errorobject_setnomem(interp, errptr);
		return STATUS_ERROR;
	}

	// don't decref func, now klass->data->methods holds the reference
	// don't free uname or uname->val because uname is now used as a key in info->methods
	return STATUS_OK;
}

// the base class is an ObjectClassInfo for some reason
// that's why the recursive stuff needs to work with ObjectClassInfo
static struct Object *get_the_method(struct ObjectClassInfo *classinfo, struct UnicodeString *uname, unsigned int unamehash)
{
	struct Object *res;
	if (hashtable_get(classinfo->methods, uname, unamehash, (void **)(&res), NULL))
		return res;

	// inheritance: maybe this is defined in a base class?
	if (classinfo->baseclass)
		return get_the_method(classinfo->baseclass, uname, unamehash);

	// nope
	return NULL;
}

struct Object *method_getwithustr(struct Interpreter *interp, struct Object **errptr, struct Object *obj, struct UnicodeString uname)
{
	struct Object *nopartial = get_the_method(obj->klass->data, &uname, unicodestring_hash(uname));
	if (!nopartial) {
		assert(0);
		// FIXME: this thing needs a ctx
		/*
		char *classname = ((struct ObjectClassInfo*) obj->klass->data)->name;
		errorobject_setwithfmt(ctx, errptr,
			"%s objects don't have %s '%U' method",
			classname,
			(uname.len >= 1 && unicode_iswovel(uname.val[0])) ? "an" : "a",
			uname);
		*/
		return NULL;
	}

	// now we have a function that takes self as the first argument, let's partial it
	// no need to decref nopartial because get_the_method() doesn't incref
	return functionobject_newpartial(interp, errptr, nopartial, obj);
}

struct Object *method_get(struct Interpreter *interp, struct Object **errptr, struct Object *obj, char *name)
{
	struct UnicodeString *uname = malloc(sizeof(struct UnicodeString));
	if (!uname) {
		errorobject_setnomem(interp, errptr);
		return NULL;
	}

	char errormsg[100];
	int status = utf8_decode(name, strlen(name), uname, errormsg);
	if (status != STATUS_OK) {
		assert(status == STATUS_NOMEM);
		free(uname);
		errorobject_setnomem(interp, errptr);
		return NULL;
	}

	struct Object *res = method_getwithustr(interp, errptr, obj, *uname);
	free(uname->val);
	free(uname);
	return res;
}


#define NARGS_MAX 20   // same as in objects/function.c
struct Object *method_call(struct Context *ctx, struct Object **errptr, struct Object *obj, char *methname, ...)
{
	struct Object *method = method_get(ctx->interp, errptr, obj, methname);
	if (!method)
		return NULL;

	struct Object *args[NARGS_MAX];
	va_list ap;
	va_start(ap, methname);

	int nargs;
	for (nargs = 0; nargs < NARGS_MAX; nargs++) {
		struct Object *arg = va_arg(ap, struct Object*);
		if (!arg)
			break;   // end of argument list, not an error
		args[nargs] = arg;
	}
	va_end(ap);

	struct Object *res = functionobject_vcall(ctx, errptr, method, args, nargs);
	OBJECT_DECREF(ctx->interp, method);
	return res;
}
#undef NARGS_MAX

static struct Object *to_maybe_debug_string(struct Context *ctx, struct Object **errptr, struct Object *obj, char *methname)
{
	struct Object *stringclass = interpreter_getbuiltin(ctx->interp, errptr, "String");
	if (!stringclass)
		return NULL;

	struct Object *res = method_call(ctx, errptr, obj, methname, NULL);
	if (!res) {
		OBJECT_DECREF(ctx->interp, stringclass);
		return NULL;
	}

	// this doesn't use errorobject_typecheck() because this uses a custom error message string
	// TODO: test this
	if (!classobject_istypeof(stringclass, res)) {
		// FIXME: is it possible to make this recurse infinitely by returning the object itself from to_{debug,}string?
		errorobject_setwithfmt(ctx, errptr, "%s should return a String, but it returned %D", methname, res);
		return NULL;
	}
	OBJECT_DECREF(ctx->interp, stringclass);
	assert(res->klass == stringclass);   // TODO: better error handling
	return res;
}

struct Object *method_call_tostring(struct Context *ctx, struct Object **errptr, struct Object *obj)
{
	return to_maybe_debug_string(ctx, errptr, obj, "to_string");
}

struct Object *method_call_todebugstring(struct Context *ctx, struct Object **errptr, struct Object *obj)
{
	return to_maybe_debug_string(ctx, errptr, obj, "to_debug_string");
}
