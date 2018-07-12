#include "method.h"
#include <assert.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include "attribute.h"
#include "check.h"
#include "interpreter.h"
#include "objects/array.h"
#include "objects/classobject.h"
#include "objects/errors.h"
#include "objects/function.h"
#include "objects/mapping.h"
#include "objectsystem.h"

// these names are only for functions created in C
#define NAME_SIZE 100


struct MethodGetterData {
	struct Object *klass;
	struct FunctionObjectCfunc cfunc;
	char name[NAME_SIZE];    // ends with " method", including the \0
};

void mgdata_foreachref(void *data, object_foreachrefcb cb, void *cbdata)
{
	struct MethodGetterData *mgdata = data;
	cb(mgdata->klass, cbdata);
}

void mgdata_destructor(void *data)
{
	free(data);
}

void thisdata_foreachref(void *data, object_foreachrefcb cb, void *cbdata)
{
	cb((struct Object*) data, cbdata);
}

// when getting the method, this is called with the instance as an argument, see objects/classobject.h
static struct Object *method_getter(struct Interpreter *interp, struct ObjectData objdata, struct Object *args, struct Object *opts)
{
	struct MethodGetterData *mgdata = objdata.data;
	if (!check_args(interp, args, mgdata->klass, NULL)) return NULL;
	if (!check_no_opts(interp, opts)) return NULL;

	struct ObjectData thisdata = { .data=ARRAYOBJECT_GET(args, 0), .foreachref=thisdata_foreachref, .destructor=NULL };
	struct Object *res = functionobject_new(interp, thisdata, mgdata->cfunc, mgdata->name);
	if (!res)
		return NULL;
	OBJECT_INCREF(interp, (struct Object*) thisdata.data);   // for thisdata

	return res;
}

static bool raw_method_add(struct Interpreter *interp, struct Object *klass, char *name, struct FunctionObjectCfunc cfunc)
{
	// the instance-specific method returned by the getter will have this name
	size_t len = strlen(name);
#define MAX(a, b) ((a)>(b) ? (a) : (b))
	assert(len + MAX(sizeof(" method"), sizeof("getter of ")) <= NAME_SIZE);  // both sides include \0
#undef MAX

	struct MethodGetterData *mgdata = malloc(sizeof(struct MethodGetterData));
	if (!mgdata) {
		errorobject_thrownomem(interp);
		return false;
	}

	mgdata->klass = klass;
	OBJECT_INCREF(interp, klass);
	mgdata->cfunc = cfunc;
	memcpy(mgdata->name, name, len);
	memcpy(mgdata->name+len, " method", sizeof(" method"));   // includes \0

	char mgname[NAME_SIZE];
	memcpy(mgname, "getter of ", sizeof("getter of ")-1);
	memcpy(mgname + sizeof("getter of ")-1, name, len+1 /* also copy \0 */);

	struct Object *mg = functionobject_new(interp, (struct ObjectData){.data=mgdata, .foreachref=mgdata_foreachref, .destructor=mgdata_destructor}, functionobject_mkcfunc_yesret(method_getter), mgname);
	if (!mg) {
		OBJECT_DECREF(interp, klass);
		free(mgdata);
		return false;
	}

	bool ok = attribute_addwithfuncobjs(interp, klass, name, mg, NULL);
	OBJECT_DECREF(interp, mg);
	return ok;
}

bool method_add_yesret(struct Interpreter *interp, struct Object *klass, char *name, functionobject_cfunc_yesret cfunc)
{
	return raw_method_add(interp, klass, name, functionobject_mkcfunc_yesret(cfunc));
}

bool method_add_noret(struct Interpreter *interp, struct Object *klass, char *name, functionobject_cfunc_noret cfunc)
{
	struct FunctionObjectCfunc foc = { .returning = false };
	foc.func.noret = cfunc;
	return raw_method_add(interp, klass, name, foc);
}


// res should be a C array of method, args, opts
static bool method_call_helper(struct Interpreter *interp, struct Object *obj, char *methname, va_list ap, struct Object **res)
{
	if (!(res[0] = attribute_get(interp, obj, methname)))
		return false;
	if (!check_type(interp, interp->builtins.Function, res[0])) {
		OBJECT_DECREF(interp, res[0]);
		return false;
	}

	if (!(res[1] = arrayobject_newempty(interp))) {
		OBJECT_DECREF(interp, res[1]);
		return false;
	}
	struct Object *arg;
	while((arg = va_arg(ap, struct Object*))) {
		if (!arrayobject_push(interp, res[1], arg)) {
			OBJECT_DECREF(interp, res[1]);
			OBJECT_DECREF(interp, res[0]);
			return false;
		}
	}

	if (!(res[2] = mappingobject_newempty(interp))) {
		OBJECT_DECREF(interp, res[1]);
		OBJECT_DECREF(interp, res[0]);
		return false;
	}

	return true;
}

struct Object *method_call_yesret(struct Interpreter *interp, struct Object *obj, char *methname, ...)
{
	// i think some man page has said that va_start may contain { and va_end may contain }
	// so don't declare these between va_start and va_end
	struct Object *callargs[3];
	bool ok;

	va_list ap;
	va_start(ap, methname);
	ok = method_call_helper(interp, obj, methname, ap, callargs);
	va_end(ap);
	if (!ok)
		return NULL;

	struct Object *res = functionobject_vcall_yesret(interp, callargs[0], callargs[1], callargs[2]);
	OBJECT_DECREF(interp, callargs[0]);
	OBJECT_DECREF(interp, callargs[1]);
	OBJECT_DECREF(interp, callargs[2]);
	return res;
}

bool method_call_noret(struct Interpreter *interp, struct Object *obj, char *methname, ...)
{
	struct Object *callargs[3];
	bool ok;     

	va_list ap;
	va_start(ap, methname);
	ok = method_call_helper(interp, obj, methname, ap, callargs);
	va_end(ap);
	if (!ok)
		return false;

	ok = functionobject_vcall_noret(interp, callargs[0], callargs[1], callargs[2]);
	OBJECT_DECREF(interp, callargs[0]);
	OBJECT_DECREF(interp, callargs[1]);
	OBJECT_DECREF(interp, callargs[2]);
	return ok;
}


static struct Object *to_maybe_debug_string(struct Interpreter *interp, struct Object *obj, char *methname)
{
	struct Object *res = method_call_yesret(interp, obj, methname, NULL);
	if (!res)
		return NULL;
	if (!classobject_isinstanceof(res, interp->builtins.String)) {
		// this error message is better than the one from check_type()
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

