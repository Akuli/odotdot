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


struct Object *scopeobject_newsub(struct Interpreter *interp, struct Object *parent_scope)
{
	assert(interp->builtins.Scope);

	struct Object *scope = classobject_newinstance(interp, interp->builtins.Scope, NULL, NULL);
	if (!scope)
		return NULL;

	struct Object *local_vars = mappingobject_newempty(interp);
	if (!local_vars) {
		OBJECT_DECREF(interp, scope);
		return NULL;
	}

	if (!attribute_settoattrdata(interp, scope, "parent_scope", parent_scope)) goto error;
	if (!attribute_settoattrdata(interp, scope, "local_vars", local_vars)) goto error;

	OBJECT_DECREF(interp, local_vars);
	return scope;

error:
	OBJECT_DECREF(interp, local_vars);
	OBJECT_DECREF(interp, scope);
	return NULL;
}

struct Object *scopeobject_newbuiltin(struct Interpreter *interp)
{
	// null is defined in stdlib/builtins.ö, but this is called from builtins.c before builtins.ö runs
	// builtins.ö replaces this with the correct null object
	struct Object *fakenull = classobject_newinstance(interp, interp->builtins.Scope, NULL, NULL);
	if (!fakenull)
		return NULL;

	struct Object *res = scopeobject_newsub(interp, fakenull);
	OBJECT_DECREF(interp, fakenull);
	return res;
}


ATTRIBUTE_DEFINE_SIMPLE_GETTER(parent_scope, Scope)
ATTRIBUTE_DEFINE_SIMPLE_GETTER(local_vars, Scope)

static struct Object *setup(struct Interpreter *interp, struct Object *args, struct Object *opts)
{
	if (!check_args(interp, args, interp->builtins.Scope, interp->builtins.Scope, NULL)) return NULL;
	if (!check_no_opts(interp, opts)) return NULL;
	struct Object *scope = ARRAYOBJECT_GET(args, 0);
	struct Object *parent_scope = ARRAYOBJECT_GET(args, 1);

	struct Object *local_vars = mappingobject_newempty(interp);
	if (!local_vars)
		return NULL;

	if (!attribute_settoattrdata(interp, scope, "parent_scope", parent_scope)) goto error;
	if (!attribute_settoattrdata(interp, scope, "local_vars", local_vars)) goto error;

	OBJECT_DECREF(interp, local_vars);
	return nullobject_get(interp);

error:
	OBJECT_DECREF(interp, local_vars);
	return NULL;
}


static struct Object *set_var(struct Interpreter *interp, struct Object *args, struct Object *opts)
{
	if (!check_args(interp, args, interp->builtins.Scope, interp->builtins.String, interp->builtins.Object, NULL)) return NULL;
	if (!check_no_opts(interp, opts)) return NULL;
	struct Object *scope = ARRAYOBJECT_GET(args, 0);
	struct Object *varname = ARRAYOBJECT_GET(args, 1);
	struct Object *val = ARRAYOBJECT_GET(args, 2);

	struct Object *local_vars = attribute_get(interp, scope, "local_vars");
	if (!local_vars)
		return NULL;

	// is the variable defined here?
	struct Object *oldval;
	int res = mappingobject_get(interp, local_vars, varname, &oldval);
	OBJECT_DECREF(interp, local_vars);
	if (res == 1) {
		// yes
		OBJECT_DECREF(interp, oldval);
		return mappingobject_set(interp, local_vars, varname, val) ? nullobject_get(interp) : NULL;
	}
	if (res == -1)
		return NULL;
	assert(res == 0);    // not found

	// but do we have a parent scope?
	if (scope == interp->builtinscope) {
		// no, all scopes were already checked
		errorobject_setwithfmt(interp, "VariableError", "no variable named %D", varname);
		return NULL;
	}

	// maybe the variable is defined in the parent scope?
	struct Object *parent_scope = attribute_get(interp, scope, "parent_scope");
	if (!parent_scope)
		return NULL;
	struct Object *tmp[] = { parent_scope, varname, val };
	struct Object *newargs = arrayobject_new(interp, tmp, 3);
	OBJECT_DECREF(interp, parent_scope);
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
	// TODO: better error message for missing variables
	struct Object *local_vars = attribute_get(interp, scope, "local_vars");
	if (!local_vars)
		return NULL;
	struct Object *val;
	int res = mappingobject_get(interp, local_vars, varname, &val);
	OBJECT_DECREF(interp, local_vars);
	if (res == 1)
		return val;
	if (res == -1)
		return NULL;
	assert(res == 0);   // not found

	if (scope == interp->builtinscope) {
		errorobject_setwithfmt(interp, "VariableError", "no variable named %D", varname);
		return NULL;
	}

	struct Object *parent_scope = attribute_get(interp, scope, "parent_scope");
	if (!parent_scope)
		return NULL;
	struct Object *tmp[] = { parent_scope, varname };
	struct Object *newargs = arrayobject_new(interp, tmp, 2);
	OBJECT_DECREF(interp, parent_scope);
	if (!newargs)
		return NULL;

	struct Object *ret = get_var(interp, newargs, opts);
	OBJECT_DECREF(interp, newargs);
	return ret;
}

struct Object *scopeobject_createclass(struct Interpreter *interp)
{
	struct Object *klass = classobject_new(interp, "Scope", interp->builtins.Object, NULL, false);
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
