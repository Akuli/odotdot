#include "method.h"
#include <assert.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include "attribute.h"
#include "check.h"
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

// this is partialled to function objects when creating methods
// when getting the method, the partialled thing is called with the instance as an argument, see objects/classobject.h
static struct Object *method_getter(struct Interpreter *interp, struct Object *argarr)
{
	if (check_args(interp, argarr, interp->builtins.functionclass, interp->builtins.objectclass, NULL) == STATUS_ERROR)
		return NULL;
	return functionobject_newpartial(interp, ARRAYOBJECT_GET(argarr, 0), ARRAYOBJECT_GET(argarr, 1));
}

int method_add(struct Interpreter *interp, struct Object *klass, char *name, functionobject_cfunc cfunc)
{
	struct Object *func = functionobject_new(interp, cfunc);
	if (!func)
		return STATUS_ERROR;

	struct Object *rawgetter = functionobject_new(interp, method_getter);
	if (!rawgetter) {
		OBJECT_DECREF(interp, func);
		return STATUS_ERROR;
	}

	struct Object *getter = functionobject_newpartial(interp, rawgetter, func);
	OBJECT_DECREF(interp, rawgetter);
	OBJECT_DECREF(interp, func);
	if (!getter)
		return STATUS_ERROR;

	int res = attribute_addwithfuncobjs(interp, klass, name, getter, NULL);
	OBJECT_DECREF(interp, getter);
	return res;
}


struct Object *method_call(struct Interpreter *interp, struct Object *obj, char *methname, ...)
{
	struct Object *method = attribute_get(interp, obj, methname);
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
	if (!classobject_isinstanceof(res, interp->builtins.stringclass)) {
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

