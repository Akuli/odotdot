#include "method.h"
#include <assert.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include "attribute.h"
#include "check.h"
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
static struct Object *method_getter(struct Interpreter *interp, struct Object *args, struct Object *opts)
{
	if (!check_args(interp, args, interp->builtins.Function, interp->builtins.Object, NULL)) return NULL;
	if (!check_no_opts(interp, opts)) return NULL;
	return functionobject_newpartial(interp, ARRAYOBJECT_GET(args, 0), ARRAYOBJECT_GET(args, 1));
}

bool method_add(struct Interpreter *interp, struct Object *klass, char *name, functionobject_cfunc cfunc)
{
	// the instance-specific method returned by the getter will have this name
	size_t len = strlen(name);
	char methodname[len + sizeof(" method") /* includes \0 */];
	memcpy(methodname, name, len);
	strcpy(methodname+len, " method");

	struct Object *func = functionobject_new(interp, cfunc, methodname);
	if (!func)
		return false;

	// getters are named as in attribute.c
	char gettername[sizeof("getter of ")-1 + len + 1];
	memcpy(gettername, "getter of ", sizeof("getter of ")-1);
	strcpy(gettername + sizeof("getter of ")-1, name);

	struct Object *rawgetter = functionobject_new(interp, method_getter, gettername);
	if (!rawgetter) {
		OBJECT_DECREF(interp, func);
		return false;
	}

	struct Object *getter = functionobject_newpartial(interp, rawgetter, func);
	OBJECT_DECREF(interp, rawgetter);
	OBJECT_DECREF(interp, func);
	if (!getter)
		return false;

	bool ok = attribute_addwithfuncobjs(interp, klass, name, getter, NULL);
	OBJECT_DECREF(interp, getter);
	return ok;
}


struct Object *method_call(struct Interpreter *interp, struct Object *obj, char *methname, ...)
{
	struct Object *method = attribute_get(interp, obj, methname);
	if (!method)
		return NULL;
	if (!check_type(interp, interp->builtins.Function, method)) {
		OBJECT_DECREF(interp, method);
		return NULL;
	}

	struct Object *args = arrayobject_newempty(interp);
	if (!args) {
		OBJECT_DECREF(interp, method);
		return NULL;
	}

	va_list ap;
	va_start(ap, methname);

	while(true) {
		struct Object *arg = va_arg(ap, struct Object*);
		if (!arg)
			break;   // end of argument list, not an error
		if (!arrayobject_push(interp, args, arg)) {
			OBJECT_DECREF(interp, args);
			OBJECT_DECREF(interp, method);
			return NULL;
		}
	}
	va_end(ap);

	struct Object *opts = mappingobject_newempty(interp);
	if (!opts) {
		OBJECT_DECREF(interp, args);
		OBJECT_DECREF(interp, method);
		return NULL;
	}

	struct Object *res = functionobject_vcall(interp, method, args, opts);
	OBJECT_DECREF(interp, opts);
	OBJECT_DECREF(interp, args);
	OBJECT_DECREF(interp, method);
	return res;
}

static struct Object *to_maybe_debug_string(struct Interpreter *interp, struct Object *obj, char *methname)
{
	struct Object *res = method_call(interp, obj, methname, NULL);
	if (!res)
		return NULL;

	// this doesn't use check_type() because this uses a custom error message string
	if (!classobject_isinstanceof(res, interp->builtins.String)) {
		// FIXME: is it possible to make this recurse infinitely by returning the object itself from to_{debug,}string?
		errorobject_throwfmt(interp, "TypeError", "%s should return a String, but it returned %D", methname, res);
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

