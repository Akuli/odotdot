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
	struct ObjectData userdata;  // passed to the function when calling, same data every time
};

static void function_foreachref(void *data, object_foreachrefcb cb, void *cbdata)
{
	struct FunctionData *fdata = data;
	cb(fdata->name, cbdata);

	// TODO: this boilerplate sucks
	if (fdata->userdata.foreachref)
		fdata->userdata.foreachref(fdata->userdata.data, cb, cbdata);
}

static void function_destructor(void *data)
{
	struct FunctionData *fdata = data;
	if (fdata->userdata.destructor)
		fdata->userdata.destructor(fdata->userdata.data);
	free(data);
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


static struct Object *name_getter(struct Interpreter *interp, struct ObjectData nulldata, struct Object *args, struct Object *opts)
{
	if (!check_args(interp, args, interp->builtins.Function, NULL)) return NULL;
	if (!check_no_opts(interp, opts)) return NULL;

	struct FunctionData *data = ARRAYOBJECT_GET(args, 0)->objdata.data;
	OBJECT_INCREF(interp, data->name);
	return data->name;
}

static struct Object *name_setter(struct Interpreter *interp, struct ObjectData nulldata, struct Object *args, struct Object *opts)
{
	if (!check_args(interp, args, interp->builtins.Function, interp->builtins.String, NULL)) return NULL;
	if (!check_no_opts(interp, opts)) return NULL;

	struct FunctionData *data = ARRAYOBJECT_GET(args, 0)->objdata.data;
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

	struct FunctionData *data = func->objdata.data;
	OBJECT_DECREF(interp, data->name);
	data->name = newnameobj;
	// no need to incref, newnameobj is already holding a reference
	return true;
}


static struct Object *setup(struct Interpreter *interp, struct ObjectData thisdata, struct Object *args, struct Object *opts)
{
	// FIXME: ValueError feels wrong for this
	errorobject_throwfmt(interp, "ValueError", "functions can't be created with (new Function), use func instead");
	return NULL;
}


bool functionobject_addmethods(struct Interpreter *interp)
{
	if (!attribute_add(interp, interp->builtins.Function, "name", name_getter, name_setter)) return false;
	if (!method_add(interp, interp->builtins.Function, "setup", setup)) return false;
	return true;
}


struct Object *functionobject_new(struct Interpreter *interp, struct ObjectData userdata, functionobject_cfunc cfunc, char *name)
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
	data->userdata = userdata;

	struct Object *obj = object_new_noerr(interp, interp->builtins.Function, (struct ObjectData){.data=data, .foreachref=function_foreachref, .destructor=function_destructor});
	if (!obj) {
		errorobject_thrownomem(interp);
		OBJECT_DECREF(interp, nameobj);
		free(data);
		return NULL;
	}

	return obj;
}

bool functionobject_add2array(struct Interpreter *interp, struct Object *arr, char *name, functionobject_cfunc cfunc)
{
	struct Object *func = functionobject_new(interp, (struct ObjectData){.data=NULL, .foreachref=NULL, .destructor=NULL}, cfunc, name);
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
	struct FunctionData *fdata = func->objdata.data;
	return fdata->cfunc(interp, fdata->userdata, args, opts);
}
