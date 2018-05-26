#include "method.h"
#include <assert.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include "common.h"
#include "interpreter.h"
#include "objects/array.h"
#include "objects/classobject.h"
#include "objects/errors.h"
#include "objects/function.h"
#include "objects/integer.h"
#include "objects/mapping.h"
#include "objects/string.h"
#include "objectsystem.h"
#include "unicode.h"

int method_add(struct Interpreter *interp, struct Object *klass, char *name, functionobject_cfunc cfunc)
{
	struct Object *nameobj = stringobject_newfromcharptr(interp, name);
	if (!nameobj)
		return STATUS_ERROR;

	struct Object *func = functionobject_new(interp, cfunc);
	if (!func) {
		OBJECT_DECREF(interp, nameobj);
		return STATUS_ERROR;
	}

	struct Object *methodmap = ((struct ClassObjectData *) klass->data)->methods;
	assert(methodmap);

	int setres = mappingobject_set(interp, methodmap, nameobj, func);
	OBJECT_DECREF(interp, nameobj);
	OBJECT_DECREF(interp, func);
	return setres;
}


// does NOT return a new reference
static struct Object *get_the_method(struct Interpreter *interp, struct Object *obj, struct Object *nameobj)
{
	struct Object *klass = obj->klass;
	struct ClassObjectData *klassdata;
	do {
		klassdata = klass->data;
		struct Object *nopartial = mappingobject_get(interp, klassdata->methods, nameobj);
		if (nopartial) {
			// now we have a function that takes self as the first argument, let's partial it
			struct Object *res = functionobject_newpartial(interp, nopartial, obj);
			OBJECT_DECREF(interp, nopartial);
			return res;
		}
		if (interp->err && !nopartial)
			return NULL;    // error
		// key not found, keep trying
	} while ((klass = klassdata->baseclass));

	errorobject_setwithfmt(interp, "%s objects don't have a method named '%S'", ((struct ClassObjectData *) obj->klass->data)->name, nameobj);
	return NULL;
}

struct Object *method_getwithustr(struct Interpreter *interp, struct Object *obj, struct UnicodeString uname)
{
	struct Object *nameobj = stringobject_newfromustr(interp, uname);
	if (!nameobj)
		return NULL;

	struct Object *res = get_the_method(interp, obj, nameobj);
	OBJECT_DECREF(interp, nameobj);
	return res;
}

struct Object *method_get(struct Interpreter *interp, struct Object *obj, char *name)
{
	struct Object *nameobj = stringobject_newfromcharptr(interp, name);
	if (!nameobj)
		return NULL;

	struct Object *res = get_the_method(interp, obj, nameobj);
	OBJECT_DECREF(interp, nameobj);
	return res;
}


struct Object *method_call(struct Interpreter *interp, struct Object *obj, char *methname, ...)
{
	struct Object *method = method_get(interp, obj, methname);
	if (!method)
		return NULL;

	struct Object *argarr = arrayobject_newempty(interp);
	if (!argarr) {
		OBJECT_DECREF(interp, method);
		return NULL;
	}

	va_list ap;
	va_start(ap, methname);

	while(true) {
		struct Object *arg = va_arg(ap, struct Object*);
		if (!arg)
			break;   // end of argument list, not an error
		if (arrayobject_push(interp, argarr, arg) == STATUS_ERROR) {
			OBJECT_DECREF(interp, argarr);
			OBJECT_DECREF(interp, method);
			return NULL;
		}
	}
	va_end(ap);

	struct Object *res = functionobject_vcall(interp, method, argarr);
	OBJECT_DECREF(interp, argarr);
	OBJECT_DECREF(interp, method);
	return res;
}

static struct Object *to_maybe_debug_string(struct Interpreter *interp, struct Object *obj, char *methname)
{
	struct Object *res = method_call(interp, obj, methname, NULL);
	if (!res)
		return NULL;

	// this doesn't use check_type() because this uses a custom error message string
	if (!classobject_instanceof(res, interp->builtins.stringclass)) {
		// FIXME: is it possible to make this recurse infinitely by returning the object itself from to_{debug,}string?
		errorobject_setwithfmt(interp, "%s should return a String, but it returned %D", methname, res);
		OBJECT_DECREF(interp, res);
		return NULL;
	}
	return res;
}

struct Object *method_call_tostring(struct Interpreter *interp, struct Object *obj)
{
	return to_maybe_debug_string(interp, obj, "to_string");
}

struct Object *method_call_todebugstring(struct Interpreter *interp, struct Object *obj)
{
	return to_maybe_debug_string(interp, obj, "to_debug_string");
}

