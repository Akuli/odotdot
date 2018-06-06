#include "scope.h"
#include <assert.h>
#include <stdlib.h>
#include "../attribute.h"
#include "../check.h"
#include "../common.h"
#include "../method.h"
#include "../objectsystem.h"
#include "array.h"
#include "classobject.h"
#include "errors.h"
#include "function.h"
#include "mapping.h"
#include "string.h"


ATTRIBUTE_DEFINE_SIMPLE_GETTER(parent_scope, Scope)
ATTRIBUTE_DEFINE_SIMPLE_GETTER(local_vars, Scope)

/* this is a weird thing
builtins.ö sets the parent scope of the built-in scope to null once
because it creates null and the built-in scope is created before it runs
that's why this temporary thing deletes itself after it's used
*/
static struct Object *temporary_parent_scope_setter(struct Interpreter *interp, struct Object *argarr)
{
	if (check_args(interp, argarr, interp->builtins.Scope, interp->builtins.Object, NULL) == STATUS_ERROR)
		return NULL;

	struct Object *scope = ARRAYOBJECT_GET(argarr, 0);
	struct Object *null = ARRAYOBJECT_GET(argarr, 1);
	assert(scope == interp->builtinscope);

	if (attribute_settoattrdata(interp, scope, "parent_scope", null) == STATUS_ERROR)
		return NULL;

	// let's wipe this thing out
	struct ClassObjectData *classdata = scope->klass->data;
	assert(classdata->setters);

	struct Object *stringobj = stringobject_newfromcharptr(interp, "parent_scope");
	if (!stringobj)
		return NULL;

	// the mapping delete method isn't exposed, but that's ok imo
	struct Object *ret = method_call(interp, classdata->setters, "delete", stringobj, NULL);
	OBJECT_DECREF(interp, stringobj);
	return ret;
}


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

	if (attribute_settoattrdata(interp, scope, "parent_scope", parent_scope) == STATUS_ERROR) goto error;
	if (attribute_settoattrdata(interp, scope, "local_vars", local_vars) == STATUS_ERROR) goto error;

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

static struct Object *setup(struct Interpreter *interp, struct Object *argarr)
{
	if (check_args(interp, argarr, interp->builtins.Scope, interp->builtins.Scope, NULL) == STATUS_ERROR) return NULL;
	struct Object *scope = ARRAYOBJECT_GET(argarr, 0);
	struct Object *parent_scope = ARRAYOBJECT_GET(argarr, 1);

	struct Object *local_vars = mappingobject_newempty(interp);
	if (!local_vars)
		return NULL;

	if (attribute_settoattrdata(interp, scope, "parent_scope", parent_scope) == STATUS_ERROR) goto error;
	if (attribute_settoattrdata(interp, scope, "local_vars", local_vars) == STATUS_ERROR) goto error;

	OBJECT_DECREF(interp, local_vars);
	return interpreter_getbuiltin(interp, "null");

error:
	OBJECT_DECREF(interp, local_vars);
	return NULL;
}


static struct Object *set_var(struct Interpreter *interp, struct Object *argarr)
{
	if (check_args(interp, argarr, interp->builtins.Scope, interp->builtins.String, interp->builtins.Object, NULL) == STATUS_ERROR)
		return NULL;
	struct Object *scope = ARRAYOBJECT_GET(argarr, 0);
	struct Object *varname = ARRAYOBJECT_GET(argarr, 1);
	struct Object *val = ARRAYOBJECT_GET(argarr, 2);

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
		int status = mappingobject_set(interp, local_vars, varname, val);
		return status==STATUS_OK ? interpreter_getbuiltin(interp, "null") : NULL;
	}
	if (res == -1)
		return NULL;
	assert(res == 0);    // not found

	// but do we have a parent scope?
	if (scope == interp->builtinscope) {
		// no, all scopes were already checked
		errorobject_setwithfmt(interp, "no variable named '%S'", varname);
		return NULL;
	}

	// maybe the variable is defined in the parent scope?
	struct Object *parent_scope = attribute_get(interp, scope, "parent_scope");
	if (!parent_scope)
		return NULL;
	struct Object *tmp[] = { parent_scope, varname, val };
	struct Object *newargarr = arrayobject_new(interp, tmp, 3);
	OBJECT_DECREF(interp, parent_scope);
	if (!newargarr)
		return NULL;

	struct Object *ret = set_var(interp, newargarr);
	OBJECT_DECREF(interp, newargarr);
	return ret;
}

static struct Object *get_var(struct Interpreter *interp, struct Object *argarr)
{
	if (check_args(interp, argarr, interp->builtins.Scope, interp->builtins.String, NULL) == STATUS_ERROR)
		return NULL;
	struct Object *scope = ARRAYOBJECT_GET(argarr, 0);
	struct Object *varname = ARRAYOBJECT_GET(argarr, 1);

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
		errorobject_setwithfmt(interp, "no variable named '%S'", varname);
		return NULL;
	}

	struct Object *parent_scope = attribute_get(interp, scope, "parent_scope");
	if (!parent_scope)
		return NULL;
	struct Object *tmp[] = { parent_scope, varname };
	struct Object *newargarr = arrayobject_new(interp, tmp, 2);
	OBJECT_DECREF(interp, parent_scope);
	if (!newargarr)
		return NULL;

	struct Object *ret = get_var(interp, newargarr);
	OBJECT_DECREF(interp, newargarr);
	return ret;
}

struct Object *scopeobject_createclass(struct Interpreter *interp)
{
	struct Object *klass = classobject_new(interp, "Scope", interp->builtins.Object, NULL);
	if (!klass)
		return NULL;

	if (method_add(interp, klass, "setup", setup) == STATUS_ERROR) goto error;
	if (method_add(interp, klass, "set_var", set_var) == STATUS_ERROR) goto error;
	if (method_add(interp, klass, "get_var", get_var) == STATUS_ERROR) goto error;
	if (attribute_add(interp, klass, "local_vars", local_vars_getter, NULL) == STATUS_ERROR) goto error;
	if (attribute_add(interp, klass, "parent_scope", parent_scope_getter, temporary_parent_scope_setter) == STATUS_ERROR) goto error;
	return klass;

error:
	OBJECT_DECREF(interp, klass);
	return NULL;
}
