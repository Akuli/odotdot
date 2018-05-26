#include "catch.h"
#include <assert.h>
#include "../attribute.h"
#include "../check.h"
#include "../common.h"
#include "../interpreter.h"
#include "../method.h"
#include "../objectsystem.h"
#include "../objects/array.h"
#include "../objects/scope.h"

static int run_block(struct Interpreter *interp, struct Object *block)
{
	struct Object *defscope = attribute_get(interp, block, "definition_scope");
	if (!defscope)
		return STATUS_ERROR;

	struct Object *subscope = scopeobject_newsub(interp, defscope);
	OBJECT_DECREF(interp, defscope);
	if (!subscope)
		return STATUS_ERROR;

	struct Object *res = method_call(interp, block, "run", subscope, NULL);
	OBJECT_DECREF(interp, subscope);
	if (!res)
		return STATUS_ERROR;

	OBJECT_DECREF(interp, res);
	return STATUS_OK;
}

struct Object *builtin_catch(struct Interpreter *interp, struct Object *argarr)
{
	if (check_args(interp, argarr, interp->builtins.blockclass, interp->builtins.blockclass, NULL) == STATUS_ERROR)
		return NULL;
	struct Object *trying = ARRAYOBJECT_GET(argarr, 0);
	struct Object *caught = ARRAYOBJECT_GET(argarr, 1);

	if (run_block(interp, trying) == STATUS_ERROR) {
		// TODO: make the error available somewhere instead of resetting it here?
		assert(interp->err);
		OBJECT_DECREF(interp, interp->err);
		interp->err = NULL;

		if (run_block(interp, caught) == STATUS_ERROR)
			return NULL;
	}

	// everything succeeded or the error handling code succeeded
	return interpreter_getbuiltin(interp, "null");
}
