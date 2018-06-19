#include "scope.h"
#include <assert.h>
#include <stdbool.h>
#include <stdlib.h>
#include "../attribute.h"
#include "../check.h"
#include "../method.h"
#include "../objectsystem.h"
#include "array.h"
#include "classobject.h"
#include "errors.h"
#include "function.h"
#include "mapping.h"
#include "null.h"
#include "string.h"

static void builtin_scope_foreachref(struct Object *scope, void *cbdata, object_foreachrefcb cb)
{
	struct ScopeObjectData *data = scope->data;
	if (data)
		cb(data->local_vars, cbdata);
}

static void subscope_foreachref(struct Object *scope, void *cbdata, object_foreachrefcb cb)
{
	struct ScopeObjectData *data = scope->data;
	if (data) {
		cb(data->local_vars, cbdata);
		cb(data->parent_scope, cbdata);
	}
}

static void scope_destructor(struct Object *scope)
{
	if (scope->data)
		free(scope->data);
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

	struct Object *scope = object_new_noerr(interp, interp->builtins.Scope, data, subscope_foreachref, scope_destructor);
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

	struct Object *scope = object_new_noerr(interp, interp->builtins.Scope, data, builtin_scope_foreachref, scope_destructor);
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

	struct Object *scope = object_new_noerr(interp, ARRAYOBJECT_GET(args, 0), data, subscope_foreachref, scope_destructor);
	if (!scope) {
		OBJECT_DECREF(interp, data->parent_scope);
		OBJECT_DECREF(interp, data->local_vars);
		free(data);
		return NULL;
	}
	return scope;
}

// allow passing arguments to the constructor
static struct Object *setup(struct Interpreter *interp, struct Object *args, struct Object *opts)
{
	return nullobject_get(interp);
}

static struct Object *set_var(struct Interpreter *interp, struct Object *args, struct Object *opts)
{
	if (!check_args(interp, args, interp->builtins.Scope, interp->builtins.String, interp->builtins.Object, NULL)) return NULL;
	if (!check_no_opts(interp, opts)) return NULL;
	struct Object *scope = ARRAYOBJECT_GET(args, 0);
	struct Object *varname = ARRAYOBJECT_GET(args, 1);
	struct Object *val = ARRAYOBJECT_GET(args, 2);

	struct ScopeObjectData *scopedata = scope->data;
	assert(scopedata);

	// is the variable defined here?
	struct Object *oldval;
	int res = mappingobject_get(interp, scopedata->local_vars, varname, &oldval);
	if (res == 1) {
		// yes
		OBJECT_DECREF(interp, oldval);
		return mappingobject_set(interp, scopedata->local_vars, varname, val) ? nullobject_get(interp) : NULL;
	}
	if (res == -1)
		return NULL;
	assert(res == 0);    // not found

	// but do we have a parent scope?
	if (scope == interp->builtinscope) {
		// no, all scopes were already checked
		errorobject_throwfmt(interp, "VariableError", "no variable named %D", varname);
		return NULL;
	}

	// maybe the variable is defined in the parent scope?
	struct Object *newargs = arrayobject_new(interp, (struct Object *[]) { scopedata->parent_scope, varname, val }, 3);
	if (!newargs)
		return NULL;

	struct Object *ret = set_var(interp, newargs, opts);
	OBJECT_DECREF(interp, newargs);
	return ret;
}

static struct Object *get_var(struct Interpreter *interp, struct Object *args, struct Object *opts)
{
	if (!check_args(interp, args, interp->builtins.Scope, interp->builtins.String, NULL)) return NULL;
	if (!check_no_opts(interp, opts)) return NULL;
	struct Object *scope = ARRAYOBJECT_GET(args, 0);
	struct Object *varname = ARRAYOBJECT_GET(args, 1);

	// is the variable defined here?
	struct ScopeObjectData *scopedata = scope->data;
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

	struct Object *newargs = arrayobject_new(interp, (struct Object *[]) { scopedata->parent_scope, varname }, 2);
	if (!newargs)
		return NULL;

	struct Object *ret = get_var(interp, newargs, opts);
	OBJECT_DECREF(interp, newargs);
	return ret;
}


static struct Object *parent_scope_getter(struct Interpreter *interp, struct Object *args, struct Object *opts)
{
	if (!check_args(interp, args, interp->builtins.Scope, NULL)) return NULL;
	if (!check_no_opts(interp, opts)) return NULL;

	struct Object *res = ((struct ScopeObjectData *) ARRAYOBJECT_GET(args, 0)->data)->parent_scope;
	if (!res)
		res = interp->builtins.null;
	OBJECT_INCREF(interp, res);
	return res;
}

static struct Object *local_vars_getter(struct Interpreter *interp, struct Object *args, struct Object *opts)
{
	if (!check_args(interp, args, interp->builtins.Scope, NULL)) return NULL;
	if (!check_no_opts(interp, opts)) return NULL;

	struct Object *res = ((struct ScopeObjectData *) ARRAYOBJECT_GET(args, 0)->data)->local_vars;
	OBJECT_INCREF(interp, res);
	return res;
}

struct Object *scopeobject_createclass(struct Interpreter *interp)
{
	struct Object *klass = classobject_new(interp, "Scope", interp->builtins.Object, false, newinstance);
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
