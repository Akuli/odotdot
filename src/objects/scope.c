#include "scope.h"
#include <assert.h>
#include <stdbool.h>
#include <stdlib.h>
#include "../attribute.h"
#include "../check.h"
#include "../interpreter.h"
#include "../method.h"
#include "../objectsystem.h"
#include "array.h"
#include "classobject.h"
#include "errors.h"
#include "mapping.h"
#include "option.h"

static void builtin_scope_foreachref(void *data, object_foreachrefcb cb, void *cbdata)
{
	cb(((struct ScopeObjectData *)data)->local_vars, cbdata);
}

static void subscope_foreachref(void *data, object_foreachrefcb cb, void *cbdata)
{
	cb(((struct ScopeObjectData *)data)->local_vars, cbdata);
	cb(((struct ScopeObjectData *)data)->parent_scope, cbdata);
}

static void scope_destructor(void *data)
{
	free(data);
}


static struct ScopeObjectData *create_data(struct Interpreter *interp, struct Object *parent_scope)
{
	struct ScopeObjectData *data = malloc(sizeof(struct ScopeObjectData));
	if (!data) {
		errorobject_thrownomem(interp);
		return NULL;
	}

	data->local_vars = mappingobject_newempty(interp);
	if (!data->local_vars) {
	 	free(data);
		return NULL;
	}

	data->parent_scope = parent_scope;
	if (parent_scope)
		OBJECT_INCREF(interp, parent_scope);
	return data;
}

struct Object *scopeobject_newsub(struct Interpreter *interp, struct Object *parent_scope)
{
	assert(interp->builtins.Scope);
	assert(parent_scope);

	struct ScopeObjectData *data = create_data(interp, parent_scope);
	if (!data)
		return NULL;

	struct Object *scope = object_new_noerr(interp, interp->builtins.Scope, (struct ObjectData){.data=data, .foreachref=subscope_foreachref, .destructor=scope_destructor});
	if (!scope) {
		errorobject_thrownomem(interp);
		OBJECT_DECREF(interp, data->parent_scope);
		OBJECT_DECREF(interp, data->local_vars);
		free(data);
		return NULL;
	}
	return scope;
}

struct Object *scopeobject_newbuiltin(struct Interpreter *interp)
{
	assert(interp->builtins.Scope);

	struct ScopeObjectData *data = create_data(interp, NULL);
	if (!data)
		return NULL;

	struct Object *scope = object_new_noerr(interp, interp->builtins.Scope, (struct ObjectData){.data=data, .foreachref=builtin_scope_foreachref, .destructor=scope_destructor});
	if (!scope) {
		errorobject_thrownomem(interp);
		OBJECT_DECREF(interp, data->local_vars);
		free(data);
		return NULL;
	}
	return scope;
}


static struct Object *newinstance(struct Interpreter *interp, struct Object *args, struct Object *opts)
{
	if (!check_args(interp, args, interp->builtins.Class, interp->builtins.Scope, NULL)) return NULL;
	if (!check_no_opts(interp, opts)) return NULL;
	struct Object *parent_scope = ARRAYOBJECT_GET(args, 1);

	struct ScopeObjectData *data = create_data(interp, parent_scope);
	if (!data) {
		errorobject_thrownomem(interp);
		return NULL;
	}

	struct Object *scope = object_new_noerr(interp, ARRAYOBJECT_GET(args, 0), (struct ObjectData){.data=data, .foreachref=subscope_foreachref, .destructor=scope_destructor});
	if (!scope) {
		OBJECT_DECREF(interp, data->parent_scope);
		OBJECT_DECREF(interp, data->local_vars);
		free(data);
		return NULL;
	}
	return scope;
}

// allow passing arguments to the constructor
static struct Object *setup(struct Interpreter *interp, struct ObjectData thisdata, struct Object *args, struct Object *opts)
{
	return functionobject_noreturn;
}

bool scopeobject_setvar(struct Interpreter *interp, struct Object *scope, struct Object *varname, struct Object *val)
{
	struct ScopeObjectData *scopedata = scope->objdata.data;
	assert(scopedata);

	// is the variable defined here?
	struct Object *oldval;
	int res = mappingobject_get(interp, scopedata->local_vars, varname, &oldval);
	if (res == 1) {
		// yes
		OBJECT_DECREF(interp, oldval);
		return mappingobject_set(interp, scopedata->local_vars, varname, val);
	}
	if (res == -1)
		return false;
	assert(res == 0);    // not found

	// but do we have a parent scope?
	if (scope == interp->builtinscope) {
		// no, all scopes were already checked
		errorobject_throwfmt(interp, "VariableError", "no variable named %D", varname);
		return false;
	}

	// maybe the variable is defined in the parent scope?
	return scopeobject_setvar(interp, scopedata->parent_scope, varname, val);
}

static struct Object *set_var(struct Interpreter *interp, struct ObjectData thisdata, struct Object *args, struct Object *opts)
{
	if (!check_args(interp, args, interp->builtins.String, interp->builtins.Object, NULL)) return NULL;
	if (!check_no_opts(interp, opts)) return NULL;
	if (!scopeobject_setvar(interp, thisdata.data, ARRAYOBJECT_GET(args, 0), ARRAYOBJECT_GET(args, 1)))
		return NULL;
	OBJECT_INCREF(interp, interp->builtins.none);
	return interp->builtins.none;
}

struct Object *scopeobject_getvar(struct Interpreter *interp, struct Object *scope, struct Object *varname)
{
	// is the variable defined here?
	struct ScopeObjectData *scopedata = scope->objdata.data;
	struct Object *val;
	int res = mappingobject_get(interp, scopedata->local_vars, varname, &val);
	if (res == 1)
		return val;
	if (res == -1)
		return NULL;
	assert(res == 0);   // not found

	if (scope == interp->builtinscope) {
		errorobject_throwfmt(interp, "VariableError", "no variable named %D", varname);
		return NULL;
	}

	return scopeobject_getvar(interp, scopedata->parent_scope, varname);
}

static struct Object *get_var(struct Interpreter *interp, struct ObjectData thisdata, struct Object *args, struct Object *opts)
{
	if (!check_args(interp, args, interp->builtins.String, NULL)) return NULL;
	if (!check_no_opts(interp, opts)) return NULL;
	return scopeobject_getvar(interp, thisdata.data, ARRAYOBJECT_GET(args, 0));
}


static struct Object *parent_scope_getter(struct Interpreter *interp, struct ObjectData nulldata, struct Object *args, struct Object *opts)
{
	if (!check_args(interp, args, interp->builtins.Scope, NULL)) return NULL;
	if (!check_no_opts(interp, opts)) return NULL;

	struct Object *res = ((struct ScopeObjectData *) ARRAYOBJECT_GET(args, 0)->objdata.data)->parent_scope;
	if (res)
		return optionobject_new(interp, res);
	OBJECT_INCREF(interp, interp->builtins.none);
	return interp->builtins.none;
}

ATTRIBUTE_DEFINE_STRUCTDATA_GETTER(Scope, ScopeObjectData, local_vars)

struct Object *scopeobject_createclass(struct Interpreter *interp)
{
	struct Object *klass = classobject_new(interp, "Scope", interp->builtins.Object, newinstance);
	if (!klass)
		return NULL;

	if (!method_add(interp, klass, "setup", setup)) goto error;
	if (!method_add(interp, klass, "set_var", set_var)) goto error;
	if (!method_add(interp, klass, "get_var", get_var)) goto error;
	if (!attribute_add(interp, klass, "local_vars", local_vars_getter, NULL)) goto error;
	if (!attribute_add(interp, klass, "parent_scope", parent_scope_getter, NULL)) goto error;
	return klass;

error:
	OBJECT_DECREF(interp, klass);
	return NULL;
}
