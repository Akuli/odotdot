#include "arrayfunc.h"
#include <assert.h>
#include <stddef.h>
#include "../attribute.h"
#include "../common.h"
#include "../method.h"
#include "../objects/array.h"
#include "../objects/function.h"
#include "../objects/mapping.h"
#include "../objects/scope.h"
#include "../objects/string.h"

// this is partialled to a block of code to create array_funcs
static struct Object *runner(struct Interpreter *interp, struct Object *argarr)
{
	struct Object *parentscope = attribute_get(interp, ARRAYOBJECT_GET(argarr, 0), "definition_scope");
	if (!parentscope)
		return NULL;

	struct Object *scope = scopeobject_newsub(interp, parentscope);
	OBJECT_DECREF(interp, parentscope);
	if (!scope)
		return NULL;

	struct Object *localvars = NULL, *string = NULL, *array = NULL;
	if (!((localvars = attribute_get(interp, scope, "local_vars")) &&
			(string = stringobject_newfromcharptr(interp, "arguments")) &&
			(array = arrayobject_slice(interp, argarr, 1, ARRAYOBJECT_LEN(argarr))))) {
		if (localvars) OBJECT_DECREF(interp, localvars);
		if (string) OBJECT_DECREF(interp, string);
		if (array) OBJECT_DECREF(interp, array);
		OBJECT_DECREF(interp, scope);
		return NULL;
	}

	int status = mappingobject_set(interp, localvars, string, array);
	OBJECT_DECREF(interp, localvars);
	OBJECT_DECREF(interp, string);
	OBJECT_DECREF(interp, array);
	if (status == STATUS_ERROR) {
		OBJECT_DECREF(interp, scope);
		return NULL;
	}

	struct Object *res = method_call(interp, ARRAYOBJECT_GET(argarr, 0), "run", scope, NULL);
	OBJECT_DECREF(interp, scope);
	return res;
}

struct Object *builtin_arrayfunc(struct Interpreter *interp, struct Object *argarr)
{
	if (functionobject_checktypes(interp, argarr, interp->builtins.blockclass, NULL) == STATUS_ERROR)
		return NULL;
	struct Object *block = ARRAYOBJECT_GET(argarr, 0);

	struct Object *runnerobj = functionobject_new(interp, runner);
	if (!runnerobj)
		return NULL;

	struct Object *res = functionobject_newpartial(interp, runnerobj, block);
	OBJECT_DECREF(interp, runnerobj);
	return res;   // may be NULL
}
