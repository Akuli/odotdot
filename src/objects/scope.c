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

struct ScopeData {
	struct Object *parent_scope;
	struct Object *local_vars;
};

// boilerplates
static void foreachref(struct Object *obj, void *cbdata, classobject_foreachrefcb cb)
{
	if (obj->data) {
		struct ScopeData *data = obj->data;
		cb(data->parent_scope, cbdata);
		cb(data->local_vars, cbdata);
	}
}
static void destructor(struct Object *obj)
{
	if (obj->data)
		free(obj->data);
}

static struct Object *parent_scope_getter(struct Interpreter *interp, struct Object *argarr)
{
	if (check_args(interp, argarr, interp->builtins.scopeclass, NULL) == STATUS_ERROR)
		return NULL;
	struct Object *res = ((struct ScopeData *) ARRAYOBJECT_GET(argarr, 0)->data)->parent_scope;
	OBJECT_INCREF(interp, res);
	return res;
}
static struct Object *local_vars_getter(struct Interpreter *interp, struct Object *argarr)
{
	if (check_args(interp, argarr, interp->builtins.scopeclass, NULL) == STATUS_ERROR)
		return NULL;
	struct Object *res = ((struct ScopeData *) ARRAYOBJECT_GET(argarr, 0)->data)->local_vars;
	OBJECT_INCREF(interp, res);
	return res;
}

/* this is a weird thing
builtins.ö sets the parent scope of the built-in scope to null once
because it creates null and the built-in scope is created before it runs
that's why this temporary thing deletes itself after it's used
*/
static struct Object *temporary_parent_scope_setter(struct Interpreter *interp, struct Object *argarr)
{
	if (check_args(interp, argarr, interp->builtins.scopeclass, interp->builtins.objectclass, NULL) == STATUS_ERROR)
		return NULL;

	struct Object *scope = ARRAYOBJECT_GET(argarr, 0);
	struct Object *null = ARRAYOBJECT_GET(argarr, 1);
	assert(scope == interp->builtinscope);
	struct ScopeData *scopedata = scope->data;

	OBJECT_DECREF(interp, scopedata->parent_scope);
	scopedata->parent_scope = null;
	OBJECT_INCREF(interp, null);

	// let's wipe this thing out
	struct ClassObjectData *classdata = scope->klass->data;
	assert(classdata->setters);

	struct Object *stringobj = stringobject_newfromcharptr(interp, "parent_scope");
	if (!stringobj)
		return NULL;

	// the mapping delete method isn't exposed, but that's ok imo
	// delete returns NULL or a new reference
	struct Object *res = method_call(interp, classdata->setters, "delete", stringobj, NULL);
	OBJECT_DECREF(interp, stringobj);
	return res;
}


static struct ScopeData *create_data(struct Interpreter *interp, struct Object *parent_scope)
{
	struct Object *local_vars = mappingobject_newempty(interp);
	if (!local_vars)
		return NULL;

	struct ScopeData *data = malloc(sizeof(struct ScopeData));
	if (!data) {
		OBJECT_DECREF(interp, local_vars);
		errorobject_setnomem(interp);
		return NULL;
	}
	data->parent_scope = parent_scope;
	OBJECT_INCREF(interp, parent_scope);
	data->local_vars = local_vars;
	// no need to incref local_vars, now the data holds the reference

	return data;
}

struct Object *scopeobject_newsub(struct Interpreter *interp, struct Object *parent_scope)
{
	assert(interp->builtins.scopeclass);

	struct ScopeData *data = create_data(interp, parent_scope);
	if (!data)
		return NULL;

	struct Object *scope = classobject_newinstance(interp, interp->builtins.scopeclass, data, destructor);
	if (!scope) {
		OBJECT_DECREF(interp, data->parent_scope);
		OBJECT_DECREF(interp, data->local_vars);
		free(data);
		return NULL;
	}
	return scope;
}

struct Object *scopeobject_newbuiltin(struct Interpreter *interp)
{
	// null is defined in stdlib/builtins.ö, but this is called from builtins.c before builtins.ö runs
	// builtins.ö replaces this with the correct null object
	struct Object *fakenull = classobject_newinstance(interp, interp->builtins.scopeclass, NULL, NULL);
	if (!fakenull)
		return NULL;

	struct Object *res = scopeobject_newsub(interp, fakenull);
	OBJECT_DECREF(interp, fakenull);
	return res;
}

static struct Object *setup(struct Interpreter *interp, struct Object *argarr)
{
	if (check_args(interp, argarr, interp->builtins.scopeclass, interp->builtins.scopeclass, NULL) == STATUS_ERROR) return NULL;
	struct Object *scope = ARRAYOBJECT_GET(argarr, 0);
	struct Object *parent_scope = ARRAYOBJECT_GET(argarr, 1);

	if (scope->data) {
		errorobject_setwithfmt(interp, "setup was called twice");
		return NULL;
	}

	scope->data = create_data(interp, parent_scope);
	if (!scope->data)
		return NULL;
	scope->destructor = destructor;
	return interpreter_getbuiltin(interp, "null");
}


static struct Object *set_var(struct Interpreter *interp, struct Object *argarr)
{
	if (check_args(interp, argarr, interp->builtins.scopeclass, interp->builtins.stringclass, interp->builtins.objectclass, NULL) == STATUS_ERROR)
		return NULL;
	struct Object *scope = ARRAYOBJECT_GET(argarr, 0);
	struct Object *varname = ARRAYOBJECT_GET(argarr, 1);
	struct Object *val = ARRAYOBJECT_GET(argarr, 2);
	struct Object *local_vars = ((struct ScopeData *) scope->data)->local_vars;
	if (check_type(interp, interp->builtins.mappingclass, local_vars) == STATUS_ERROR)
		return NULL;

	// is the variable defined here?
	struct Object *res = mappingobject_get(interp, local_vars, varname);
	if (res) {
		// yes
		OBJECT_DECREF(interp, res);

		if (mappingobject_set(interp, local_vars, varname, val) == STATUS_ERROR)
			return NULL;
		return interpreter_getbuiltin(interp, "null");
	}

	// TODO: better error message for missing variables
	if (interp->err)
		return NULL;
	// the key wasn't found, mappingobject_get() returned NULL without setting errptr

	// but do we have a parent scope?
	if (scope == interp->builtinscope) {
		// no, all scopes were already checked
		errorobject_setwithfmt(interp, "no variable named '%S'", varname);
		return NULL;
	}

	// maybe the variable is defined in the parent scope?
	struct Object *parent_scope = ((struct ScopeData *) scope->data)->parent_scope;
	struct Object *tmp[] = { parent_scope, varname, val };
	struct Object *newargarr = arrayobject_new(interp, tmp, 3);
	if (!newargarr)
		return NULL;

	res = set_var(interp, newargarr);
	OBJECT_DECREF(interp, newargarr);
	return res;
}

static struct Object *get_var(struct Interpreter *interp, struct Object *argarr)
{
	if (check_args(interp, argarr, interp->builtins.scopeclass, interp->builtins.stringclass, NULL) == STATUS_ERROR)
		return NULL;
	struct Object *scope = ARRAYOBJECT_GET(argarr, 0);
	struct Object *varname = ARRAYOBJECT_GET(argarr, 1);
	struct Object *local_vars = ((struct ScopeData *) scope->data)->local_vars;
	struct Object *parent_scope = ((struct ScopeData *) scope->data)->parent_scope;

	// is the variable defined here?
	// TODO: better error message for missing variables
	struct Object *res = method_call(interp, local_vars, "get", varname, NULL);
	if (res)
		return res;
	OBJECT_DECREF(interp, interp->err);
	interp->err = NULL;

	if (scope == interp->builtinscope) {
		errorobject_setwithfmt(interp, "no variable named '%S'", varname);
		return NULL;
	}

	struct Object *tmp[] = { parent_scope, varname };
	struct Object *newargarr = arrayobject_new(interp, tmp, 2);
	if (!newargarr)
		return NULL;

	res = get_var(interp, newargarr);
	OBJECT_DECREF(interp, newargarr);
	return res;
}

struct Object *scopeobject_createclass(struct Interpreter *interp)
{
	struct Object *klass = classobject_new(interp, "Scope", interp->builtins.objectclass, foreachref);
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
