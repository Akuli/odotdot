#include "operators.h"
#include <assert.h>
#include "../check.h"
#include "../common.h"
#include "../equals.h"
#include "../interpreter.h"
#include "../objects/array.h"

#define BOOL(interp, x) interpreter_getbuiltin((interp), (x) ? "true" : "false")

struct Object *builtin_equals(struct Interpreter *interp, struct Object *argarr)
{
	if (check_args(interp, argarr, interp->builtins.objectclass, interp->builtins.objectclass, NULL) == STATUS_ERROR)
		return NULL;

	int res = equals(interp, ARRAYOBJECT_GET(argarr, 0), ARRAYOBJECT_GET(argarr, 1));
	if (res == -1)
		return NULL;

	assert(res == !!res);
	return BOOL(interp, res);
}

struct Object *builtin_sameobject(struct Interpreter *interp, struct Object *argarr)
{
	if (check_args(interp, argarr, interp->builtins.objectclass, interp->builtins.objectclass, NULL) == STATUS_ERROR)
		return NULL;
	return BOOL(interp, ARRAYOBJECT_GET(argarr, 0) == ARRAYOBJECT_GET(argarr, 1));
}
