#include "arrayfunc.h"
#include <assert.h>
#include "attribute.h"
#include "common.h"
#include "method.h"
#include "objects/array.h"
#include "objects/function.h"
#include "objects/mapping.h"
#include "objects/scope.h"
#include "objects/string.h"

// this is partialled to a block of code to create array_funcs
static struct Object *runner(struct Interpreter *interp, struct Object **errptr, struct Object **args, size_t nargs)
{
	// FIXME: asserts
	struct Object *parentscope = attribute_get(interp, errptr, args[0], "definition_scope");
	assert(parentscope);   // FIXME

	struct Object *scope = scopeobject_newsub(interp, errptr, parentscope);
	OBJECT_DECREF(interp, parentscope);
	assert(scope);   // FIXME

	struct Object *localvars = attribute_get(interp, errptr, scope, "local_vars");
	assert(localvars);
	struct Object *s = stringobject_newfromcharptr(interp, errptr, "arguments");
	assert(s);
	struct Object *a = arrayobject_new(interp, errptr, args+1, nargs-1);
	assert(a);
	assert(mappingobject_set(interp, errptr, localvars, s, a) == STATUS_OK);
	OBJECT_DECREF(interp, s);
	OBJECT_DECREF(interp, a);
	OBJECT_DECREF(interp, localvars);

	struct Object *res = method_call(interp, errptr, args[0], "run", scope, NULL);
	OBJECT_DECREF(interp, scope);
	return res;
}

static struct Object *arrayfunc_builtin(struct Interpreter *interp, struct Object **errptr, struct Object **args, size_t nargs)
{
	if (functionobject_checktypes(interp, errptr, args, nargs, interp->builtins.blockclass, NULL) == STATUS_ERROR)
		return NULL;
	struct Object *block = args[0];

	struct Object *runnerobj = functionobject_new(interp, errptr, runner);
	if (!runnerobj)
		return NULL;

	struct Object *res = functionobject_newpartial(interp, errptr, runnerobj, block);
	OBJECT_DECREF(interp, runnerobj);
	return res;   // may be NULL
}

struct Object *arrayfunc_create(struct Interpreter *interp, struct Object **errptr)
{
	return functionobject_new(interp, errptr, arrayfunc_builtin);
}
