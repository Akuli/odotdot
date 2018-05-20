#include "method.h"
#include <assert.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include "common.h"
#include "interpreter.h"
#include "hashtable.h"
#include "objects/classobject.h"
#include "objects/errors.h"
#include "objects/function.h"
#include "objects/integer.h"
#include "objects/mapping.h"
#include "objects/string.h"
#include "objectsystem.h"
#include "unicode.h"

int method_add(struct Interpreter *interp, struct Object **errptr, struct Object *klass, char *name, functionobject_cfunc cfunc)
{
	struct Object *nameobj = stringobject_newfromcharptr(interp, errptr, name);
	if (!nameobj)
		return STATUS_ERROR;

	struct Object *func = functionobject_new(interp, errptr, cfunc);
	if (!func) {
		OBJECT_DECREF(interp, nameobj);
		return STATUS_ERROR;
	}

	struct Object *methodmap = ((struct ClassObjectData *) klass->data)->methods;
	assert(methodmap);

	int setres = mappingobject_set(interp, errptr, methodmap, nameobj, func);
	OBJECT_DECREF(interp, nameobj);
	OBJECT_DECREF(interp, func);
	return setres;
}


// does NOT return a new reference
static struct Object *get_the_method(struct Interpreter *interp, struct Object **errptr, struct Object *obj, struct Object *nameobj)
{
	struct Object *klass = obj->klass;
	struct ClassObjectData *klassdata;
	do {
		klassdata = klass->data;
		struct Object *nopartial = mappingobject_get(interp, errptr, klassdata->methods, nameobj);
		if (nopartial) {
			// now we have a function that takes self as the first argument, let's partial it
			struct Object *res = functionobject_newpartial(interp, errptr, nopartial, obj);
			OBJECT_DECREF(interp, nopartial);
			return res;
		}
		if (!nopartial && *errptr)
			return NULL;    // error
		// key not found, keep trying
	} while ((klass = klassdata->baseclass));

	errorobject_setwithfmt(interp, errptr, "%s objects don't have a method named '%S'", ((struct ClassObjectData *) obj->klass->data)->name, nameobj);
	return NULL;
}

struct Object *method_getwithustr(struct Interpreter *interp, struct Object **errptr, struct Object *obj, struct UnicodeString uname)
{
	struct Object *nameobj = stringobject_newfromustr(interp, errptr, uname);
	if (!nameobj)
		return NULL;

	struct Object *res = get_the_method(interp, errptr, obj, nameobj);
	OBJECT_DECREF(interp, nameobj);
	return res;
}

struct Object *method_get(struct Interpreter *interp, struct Object **errptr, struct Object *obj, char *name)
{
	struct Object *nameobj = stringobject_newfromcharptr(interp, errptr, name);
	if (!nameobj)
		return NULL;

	struct Object *res = get_the_method(interp, errptr, obj, nameobj);
	OBJECT_DECREF(interp, nameobj);
	return res;
}


#define NARGS_MAX 20   // same as in objects/function.c
struct Object *method_call(struct Interpreter *interp, struct Object **errptr, struct Object *obj, char *methname, ...)
{
	struct Object *method = method_get(interp, errptr, obj, methname);
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

	struct Object *res = functionobject_vcall(interp, errptr, method, args, nargs);
	OBJECT_DECREF(interp, method);
	return res;
}
#undef NARGS_MAX

static struct Object *to_maybe_debug_string(struct Interpreter *interp, struct Object **errptr, struct Object *obj, char *methname)
{
	struct Object *res = method_call(interp, errptr, obj, methname, NULL);
	if (!res)
		return NULL;

	// this doesn't use errorobject_typecheck() because this uses a custom error message string
	if (!classobject_instanceof(res, interp->builtins.stringclass)) {
		// FIXME: is it possible to make this recurse infinitely by returning the object itself from to_{debug,}string?
		errorobject_setwithfmt(interp, errptr, "%s should return a String, but it returned %D", methname, res);
		OBJECT_DECREF(interp, res);
		return NULL;
	}
	return res;
}

struct Object *method_call_tostring(struct Interpreter *interp, struct Object **errptr, struct Object *obj)
{
	return to_maybe_debug_string(interp, errptr, obj, "to_string");
}

struct Object *method_call_todebugstring(struct Interpreter *interp, struct Object **errptr, struct Object *obj)
{
	return to_maybe_debug_string(interp, errptr, obj, "to_debug_string");
}

