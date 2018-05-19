#include "scope.h"
#include "../attribute.h"
#include "../common.h"
#include "../method.h"
#include "../objectsystem.h"
#include "classobject.h"
#include "errors.h"
#include "function.h"
#include "mapping.h"
#include "string.h"

static struct Object *set_var(struct Interpreter *interp, struct Object **errptr, struct Object **args, size_t nargs)
{
	// TODO: figure out a nice way to make sure that the first argument is a Scope
	if (functionobject_checktypes(interp, errptr, args, nargs, interp->builtins.objectclass, interp->builtins.stringclass, NULL) == STATUS_ERROR)
		return NULL;
	struct Object *scope = args[0];
	struct Object *varname = args[1];
	struct Object *val = args[2];

	struct Object *localvars = attribute_get(interp, errptr, scope, "local_vars");
	if (!localvars)
		return NULL;

	// is the variable defined here?
	struct Object *res = method_call(interp, errptr, localvars, "get", varname, NULL);
	OBJECT_DECREF(interp, localvars);
	if (res) {
		// yes
		OBJECT_DECREF(interp, res);
		return method_call(interp, errptr, localvars, "set", varname, val, NULL);
	}
	OBJECT_DECREF(interp, *errptr);
	*errptr = NULL;

	// no, but do we have a parent scope?
	if (scope == interp->builtinscope) {
		// no, all scopes were already checked
		errorobject_setwithfmt(interp, errptr, "no variable named '%S'", varname);
		return NULL;
	}

	// maybe the variable is defined in the parent scope?
	struct Object *parentscope = attribute_get(interp, errptr, scope, "parent_scope");
	if (!parentscope)
		return NULL;

	struct Object *newargs[] = { parentscope, varname, val };
	res = set_var(interp, errptr, newargs, nargs);
	OBJECT_DECREF(interp, parentscope);
	return res;
}

static struct Object *get_var(struct Interpreter *interp, struct Object **errptr, struct Object **args, size_t nargs)
{
	if (functionobject_checktypes(interp, errptr, args, nargs, interp->builtins.objectclass, interp->builtins.stringclass, NULL) == STATUS_ERROR)
		return NULL;
	struct Object *scope = args[0];
	struct Object *varname = args[1];

	struct Object *localvars = attribute_get(interp, errptr, scope, "local_vars");
	if (!localvars)
		return NULL;

	// is the variable defined here?
	struct Object *res = method_call(interp, errptr, localvars, "get", varname, NULL);
	OBJECT_DECREF(interp, localvars);
	if (res)
		return res;
	OBJECT_DECREF(interp, *errptr);
	*errptr = NULL;

	if (scope == interp->builtinscope) {
		errorobject_setwithfmt(interp, errptr, "no variable named '%S'", varname);
		return NULL;
	}

	struct Object *parentscope = attribute_get(interp, errptr, scope, "parent_scope");
	if (!parentscope)
		return NULL;

	struct Object *newargs[] = { parentscope, varname };
	res = get_var(interp, errptr, newargs, nargs);
	OBJECT_DECREF(interp, parentscope);
	return res;
}

struct Object *scopeobject_createclass(struct Interpreter *interp, struct Object **errptr)
{
	struct Object *klass = classobject_new(interp, errptr, "Scope", interp->builtins.objectclass, 1, NULL);
	if (!klass)
		return NULL;

	if (method_add(interp, errptr, klass, "set_var", set_var) == STATUS_ERROR) goto error;
	if (method_add(interp, errptr, klass, "get_var", get_var) == STATUS_ERROR) goto error;
	return klass;

error:
	OBJECT_DECREF(interp, klass);
	return NULL;
}

#include <assert.h>
#include <stdio.h>
struct Object *scopeobject_newsub(struct Interpreter *interp, struct Object **errptr, struct Object *parentscope)
{
	assert(interp);
	assert(errptr);
	assert(interp->builtins.scopeclass);
	struct Object *scope = classobject_newinstance(interp, errptr, interp->builtins.scopeclass, NULL, NULL);
	if (!scope)
		return NULL;

	struct Object *localvars = mappingobject_newempty(interp, errptr);
	if (!localvars) {
		OBJECT_DECREF(interp, scope);
		return NULL;
	}

	if (attribute_set(interp, errptr, scope, "parent_scope", parentscope) == STATUS_ERROR) goto error;
	if (attribute_set(interp, errptr, scope, "local_vars", localvars) == STATUS_ERROR) goto error;
	OBJECT_DECREF(interp, localvars);
	return scope;

error:
	OBJECT_DECREF(interp, localvars);
	OBJECT_DECREF(interp, scope);
	return NULL;
}

struct Object *scopeobject_newbuiltin(struct Interpreter *interp, struct Object **errptr)
{
	// TODO: we really need a null object.....
	struct Object *shouldBnull = stringobject_newfromcharptr(interp, errptr, "asd shit");
	if (!shouldBnull)
		return NULL;

	struct Object *res = scopeobject_newsub(interp, errptr, shouldBnull);
	OBJECT_DECREF(interp, shouldBnull);
	return res;
}
