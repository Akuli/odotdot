#include "function.h"
#include <stdarg.h>
#include <stddef.h>
#include <stdlib.h>
#include "../attribute.h"
#include "../check.h"
#include "../interpreter.h"
#include "../method.h"
#include "../objectsystem.h"
#include "array.h"
#include "bool.h"
#include "classobject.h"
#include "errors.h"
#include "mapping.h"
#include "string.h"

struct FunctionObjectCfunc functionobject_mkcfunc_yesret(functionobject_cfunc_yesret func)
{
	struct FunctionObjectCfunc cfunc = { .returning = true };
	cfunc.func.yesret = func;
	return cfunc;
}

struct FunctionObjectCfunc functionobject_mkcfunc_noret(functionobject_cfunc_noret func)
{
	struct FunctionObjectCfunc cfunc = { .returning = false };
	cfunc.func.noret = func;
	return cfunc;
}


struct FunctionData {
	struct Object *name;
	struct FunctionObjectCfunc cfunc;
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


ATTRIBUTE_DEFINE_STRUCTDATA_GETTER(Function, FunctionData, name)

bool name_setter(struct Interpreter *interp, struct ObjectData nulldata, struct Object *args, struct Object *opts)
{
	if (!check_args(interp, args, interp->builtins.Function, interp->builtins.String, NULL)) return false;
	if (!check_no_opts(interp, opts)) return false;

	struct FunctionData *data = ARRAYOBJECT_GET(args, 0)->objdata.data;
	OBJECT_DECREF(interp, data->name);
	data->name = ARRAYOBJECT_GET(args, 1);
	OBJECT_INCREF(interp, data->name);
	return true;
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


static struct Object *returning_getter(struct Interpreter *interp, struct ObjectData nulldata, struct Object *args, struct Object *opts)
{
	if (!check_args(interp, args, interp->builtins.Function, NULL)) return NULL;
	if (!check_no_opts(interp, opts)) return NULL;

	struct FunctionData *data = ARRAYOBJECT_GET(args, 0)->objdata.data;
	return boolobject_get(interp, data->cfunc.returning);
}


static bool setup(struct Interpreter *interp, struct ObjectData thisdata, struct Object *args, struct Object *opts)
{
	// FIXME: ValueError feels wrong for this
	errorobject_throwfmt(interp, "ValueError", "functions can't be created with (new Function), use func instead");
	return false;
}

// fwd dcl
static struct Object *partial(struct Interpreter *interp, struct ObjectData thisdata, struct Object *args, struct Object *opts);

bool functionobject_addmethods(struct Interpreter *interp)
{
	if (!attribute_add(interp, interp->builtins.Function, "name", name_getter, name_setter)) return false;
	if (!attribute_add(interp, interp->builtins.Function, "returning", returning_getter, NULL)) return false;
	if (!method_add_noret(interp, interp->builtins.Function, "setup", setup)) return false;
	if (!method_add_yesret(interp, interp->builtins.Function, "partial", partial)) return false;
	return true;
}


static struct Object *new_function_with_nameobj(struct Interpreter *interp, struct ObjectData userdata, struct FunctionObjectCfunc cfunc, struct Object *nameobj)
{
	struct FunctionData *data = malloc(sizeof(struct FunctionData));
	if (!data) {
		errorobject_thrownomem(interp);
		return NULL;
	}

	data->name = nameobj;
	OBJECT_INCREF(interp, nameobj);
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

struct Object *functionobject_new(struct Interpreter *interp, struct ObjectData userdata, struct FunctionObjectCfunc cfunc, char *name)
{
	struct Object *nameobj = stringobject_newfromcharptr(interp, name);
	if (!nameobj)
		return NULL;

	struct Object *func = new_function_with_nameobj(interp, userdata, cfunc, nameobj);
	OBJECT_DECREF(interp, nameobj);
	return func;
}

bool functionobject_add2array(struct Interpreter *interp, struct Object *arr, char *name, struct FunctionObjectCfunc cfunc)
{
	struct Object *func = functionobject_new(interp, (struct ObjectData){.data=NULL, .foreachref=NULL, .destructor=NULL}, cfunc, name);
	if (!func)
		return false;

	bool ok = arrayobject_push(interp, arr, func);
	OBJECT_DECREF(interp, func);
	return ok;
}


struct Object *functionobject_vcall_yesret(struct Interpreter *interp, struct Object *func, struct Object *args, struct Object *opts)
{
	struct FunctionData *fdata = func->objdata.data;
	if (!fdata->cfunc.returning) {
		errorobject_throwfmt(interp, "TypeError", "expected a returning function, got %D", func);
		return NULL;
	}
	return fdata->cfunc.func.yesret(interp, fdata->userdata, args, opts);
}

bool functionobject_vcall_noret(struct Interpreter *interp, struct Object *func, struct Object *args, struct Object *opts)
{
	struct FunctionData *fdata = func->objdata.data;
	if (fdata->cfunc.returning) {
		errorobject_throwfmt(interp, "TypeError", "expected a function that returns nothing, got %D", func);
		return false;
	}
	return fdata->cfunc.func.noret(interp, fdata->userdata, args, opts);
}


#define DEFINE_A_FUNCTION(RETURNTYPE, YESNO) \
	RETURNTYPE functionobject_call_##YESNO##ret(struct Interpreter *interp, struct Object *func, ...) \
	{ \
		struct Object *args = arrayobject_newempty(interp); \
		if (!args) \
			return NULL; \
		\
		va_list ap; \
		va_start(ap, func); \
		\
		while(true) { \
			struct Object *arg = va_arg(ap, struct Object *); \
			if (!arg)    /* end of argument list, not an error */ \
				break; \
			if (!arrayobject_push(interp, args, arg)) { \
				OBJECT_DECREF(interp, args); \
				return NULL; \
			} \
		} \
		va_end(ap); \
		\
		struct Object *opts = mappingobject_newempty(interp); \
		if (!opts) { \
			OBJECT_DECREF(interp, args); \
			return NULL; \
		} \
		\
		RETURNTYPE res = functionobject_vcall_##YESNO##ret(interp, func, args, opts); \
		OBJECT_DECREF(interp, args); \
		OBJECT_DECREF(interp, opts); \
		return res; \
	}

DEFINE_A_FUNCTION(struct Object *, yes)
DEFINE_A_FUNCTION(bool, no)
#undef DEFINE_A_FUNCTION


// rest of this file does the .partial method


struct PartialFunctionUserdata {
	struct Object *func;
	struct Object *args;
	struct Object *opts;
};

// pfud = partial function userdata
static void pfud_foreachref(void *data, object_foreachrefcb cb, void *cbdata)
{
	struct PartialFunctionUserdata *pfud = data;
	cb(pfud->func, cbdata);
	cb(pfud->args, cbdata);
	cb(pfud->opts, cbdata);
}

static void pfud_destructor(void *data) { free(data); }

// keys in m2 override keys in m1
// TODO: move this elsewhere
static struct Object *merge_mappings(struct Interpreter *interp, struct Object *m1, struct Object *m2)
{
	struct Object *res = mappingobject_newempty(interp);
	if (!res)
		return NULL;

	struct Object *m[] = { m1, m2 };
	for (int i=0; i<2; i++) {
		struct MappingObjectIter iter;
		mappingobject_iterbegin(&iter, m[i]);
		while (mappingobject_iternext(&iter)) {
			if (!mappingobject_set(interp, res, iter.key, iter.value)) {
				OBJECT_DECREF(interp, res);
				return NULL;
			}
		}
	}

	return res;
}


#define CREATE_RUNNER(RETURNTYPE, YESNO) \
	static RETURNTYPE partialrunner_##YESNO##ret(struct Interpreter *interp, struct ObjectData pfuddata, struct Object *args, struct Object *opts) \
	{ \
		struct PartialFunctionUserdata pfud = *(struct PartialFunctionUserdata*)pfuddata.data; \
		struct Object *allargs = arrayobject_concat(interp, pfud.args, args); \
		if (!allargs) \
			return NULL; \
		struct Object *allopts = merge_mappings(interp, pfud.opts, opts); \
		if (!allopts) { \
			OBJECT_DECREF(interp, allargs); \
			return NULL; \
		} \
		\
		RETURNTYPE res = functionobject_vcall_##YESNO##ret(interp, pfud.func, allargs, allopts); \
		OBJECT_DECREF(interp, allargs); \
		OBJECT_DECREF(interp, allopts); \
		return res; \
	}

CREATE_RUNNER(struct Object *, yes)
CREATE_RUNNER(bool, no)
#undef CREATE_RUNNER


static struct Object *partial(struct Interpreter *interp, struct ObjectData thisdata, struct Object *args, struct Object *opts)
{
	struct PartialFunctionUserdata *pfud = malloc(sizeof *pfud);
	if (!pfud) {
		errorobject_thrownomem(interp);
		return NULL;
	}
	pfud->func = thisdata.data;
	pfud->args = args;
	pfud->opts = opts;
	OBJECT_INCREF(interp, pfud->func);
	OBJECT_INCREF(interp, pfud->args);
	OBJECT_INCREF(interp, pfud->opts);

	struct Object *nopartial = thisdata.data;
	struct Object *partialname = stringobject_newfromfmt(interp, "partial of %S", ((struct FunctionData*)nopartial->objdata.data)->name);
	if (!partialname)
		goto error;

	struct FunctionObjectCfunc runnercfunc;
	if ((runnercfunc.returning = ((struct FunctionData*) pfud->func->objdata.data)->cfunc.returning))
		runnercfunc.func.yesret = partialrunner_yesret;
	else
		runnercfunc.func.noret = partialrunner_noret;

	struct Object *res = new_function_with_nameobj(interp, (struct ObjectData){.data=pfud, .foreachref=pfud_foreachref, .destructor=pfud_destructor}, runnercfunc, partialname);

	OBJECT_DECREF(interp, partialname);
	if (!res)
		goto error;
	return res;

error:
	OBJECT_DECREF(interp, pfud->func);
	OBJECT_DECREF(interp, pfud->args);
	OBJECT_DECREF(interp, pfud->opts);
	free(pfud);
	return NULL;
}
