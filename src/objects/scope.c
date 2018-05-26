#include "scope.h"
#include <assert.h>
#include "../attribute.h"
#include "../common.h"
#include "../method.h"
#include "../objectsystem.h"
#include "array.h"
#include "classobject.h"
#include "errors.h"
#include "function.h"
#include "mapping.h"
#include "string.h"

struct Object *scopeobject_newsub(struct Interpreter *interp, struct Object *parentscope)
{
	assert(interp);
	assert(interp->builtins.scopeclass);
	struct Object *scope = classobject_newinstance(interp, interp->builtins.scopeclass, NULL, NULL);
	if (!scope)
		return NULL;

	struct Object *localvars = mappingobject_newempty(interp);
	if (!localvars) {
		OBJECT_DECREF(interp, scope);
		return NULL;
	}

	if (attribute_set(interp, scope, "parent_scope", parentscope) == STATUS_ERROR) goto error;
	if (attribute_set(interp, scope, "local_vars", localvars) == STATUS_ERROR) goto error;
	OBJECT_DECREF(interp, localvars);
	return scope;

error:
	OBJECT_DECREF(interp, localvars);
	OBJECT_DECREF(interp, scope);
	return NULL;
}

struct Object *scopeobject_newbuiltin(struct Interpreter *interp)
{
	// TODO: we really need a null object.....
	struct Object *shouldBnull = stringobject_newfromcharptr(interp, "asd shit");
	if (!shouldBnull)
		return NULL;

	struct Object *res = scopeobject_newsub(interp, shouldBnull);
	OBJECT_DECREF(interp, shouldBnull);
	return res;
}


static struct Object *set_var(struct Interpreter *interp, struct Object *argarr)
{
	// TODO: figure out a nice way to make sure that the first argument is a Scope
	if (functionobject_checktypes(interp, argarr, interp->builtins.objectclass, interp->builtins.stringclass, NULL) == STATUS_ERROR)
		return NULL;
	struct Object *scope = ARRAYOBJECT_GET(argarr, 0);
	struct Object *varname = ARRAYOBJECT_GET(argarr, 1);
	struct Object *val = ARRAYOBJECT_GET(argarr, 2);

	struct Object *localvars = attribute_get(interp, scope, "local_vars");
	if (!localvars)
		return NULL;

	// is the variable defined here?
	struct Object *res = method_call(interp, localvars, "get", varname, NULL);
	OBJECT_DECREF(interp, localvars);
	if (res) {
		// yes
		OBJECT_DECREF(interp, res);
		return method_call(interp, localvars, "set", varname, val, NULL);
	}
	OBJECT_DECREF(interp, interp->err);
	interp->err = NULL;

	// no, but do we have a parent scope?
	if (scope == interp->builtinscope) {
		// no, all scopes were already checked
		errorobject_setwithfmt(interp, "no variable named '%S'", varname);
		return NULL;
	}

	// maybe the variable is defined in the parent scope?
	struct Object *parentscope = attribute_get(interp, scope, "parent_scope");
	if (!parentscope)
		return NULL;

	struct Object *tmp[] = { parentscope, varname, val };
	struct Object *newargarr = arrayobject_new(interp, tmp, 3);
	if (!newargarr) {
		OBJECT_DECREF(interp, parentscope);
		return NULL;
	}

	res = set_var(interp, newargarr);
	OBJECT_DECREF(interp, newargarr);
	OBJECT_DECREF(interp, parentscope);
	return res;
}

static struct Object *get_var(struct Interpreter *interp, struct Object *argarr)
{
	if (functionobject_checktypes(interp, argarr, interp->builtins.objectclass, interp->builtins.stringclass, NULL) == STATUS_ERROR)
		return NULL;
	struct Object *scope = ARRAYOBJECT_GET(argarr, 0);
	struct Object *varname = ARRAYOBJECT_GET(argarr, 1);

	struct Object *localvars = attribute_get(interp, scope, "local_vars");
	if (!localvars)
		return NULL;

	// is the variable defined here?
	struct Object *res = method_call(interp, localvars, "get", varname, NULL);
	OBJECT_DECREF(interp, localvars);
	if (res)
		return res;
	OBJECT_DECREF(interp, interp->err);
	interp->err = NULL;

	if (scope == interp->builtinscope) {
		errorobject_setwithfmt(interp, "no variable named '%S'", varname);
		return NULL;
	}

	struct Object *parentscope = attribute_get(interp, scope, "parent_scope");
	if (!parentscope)
		return NULL;

	struct Object *tmp[] = { parentscope, varname };
	struct Object *newargarr = arrayobject_new(interp, tmp, 2);
	if (!newargarr) {
		OBJECT_DECREF(interp, parentscope);
		return NULL;
	}

	res = get_var(interp, newargarr);
	OBJECT_DECREF(interp, newargarr);
	OBJECT_DECREF(interp, parentscope);
	return res;
}

struct Object *create_subscope(struct Interpreter *interp, struct Object *argarr)
{
	if (functionobject_checktypes(interp, argarr, interp->builtins.scopeclass, NULL) == STATUS_ERROR)
		return NULL;
	return scopeobject_newsub(interp, ARRAYOBJECT_GET(argarr, 0));
}

struct Object *scopeobject_createclass(struct Interpreter *interp)
{
	struct Object *klass = classobject_new(interp, "Scope", interp->builtins.objectclass, 1, NULL);
	if (!klass)
		return NULL;

	if (method_add(interp, klass, "set_var", set_var) == STATUS_ERROR) goto error;
	if (method_add(interp, klass, "get_var", get_var) == STATUS_ERROR) goto error;
	if (method_add(interp, klass, "create_subscope", create_subscope) == STATUS_ERROR) goto error;
	return klass;

error:
	OBJECT_DECREF(interp, klass);
	return NULL;
}
