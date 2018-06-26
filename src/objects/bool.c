#include "bool.h"
#include <assert.h>
#include <stdbool.h>
#include "../check.h"
#include "../interpreter.h"
#include "../method.h"
#include "../objectsystem.h"
#include "array.h"
#include "classobject.h"
#include "errors.h"

static struct Object *newinstance(struct Interpreter *interp, struct Object *args, struct Object *opts)
{
	errorobject_throwfmt(interp, "TypeError", "new Bool objects cannot be created");
	return NULL;
}

bool boolobject_create(struct Interpreter *interp, struct Object **yes, struct Object **no)
{
	assert(interp->builtins.Object);

	struct Object *klass = classobject_new(interp, "Bool", interp->builtins.Object, newinstance);
	if (!klass)
		return false;

	struct Object *yay = object_new_noerr(interp, klass, NULL, NULL, NULL);
	if (!yay) {
		OBJECT_DECREF(interp, klass);
		errorobject_thrownomem(interp);
		return false;
	}

	struct Object *nay = object_new_noerr(interp, klass, NULL, NULL, NULL);
	if (!nay) {
		OBJECT_DECREF(interp, yay);
		OBJECT_DECREF(interp, klass);
		errorobject_thrownomem(interp);
		return false;
	}

	*yes = yay;
	*no = nay;
	OBJECT_DECREF(interp, klass);   // accessible as yay->klass and nay->klass
	return true;
}

// TODO: these are a lot of boilerplate
static struct Object *eq(struct Interpreter *interp, struct Object *args, struct Object *opts)
{
	if (!check_args(interp, args, interp->builtins.Object, interp->builtins.Object, NULL)) return NULL;
	if (!check_no_opts(interp, opts)) return NULL;
	struct Object *a = ARRAYOBJECT_GET(args, 0), *b = ARRAYOBJECT_GET(args, 1);
	if (!(classobject_isinstanceof(a, interp->builtins.Bool) && classobject_isinstanceof(b, interp->builtins.Bool)))
		return nullobject_get(interp);

	struct Object *res;
	if (a == b)
		res = interp->builtins.yes;
	else
		res = interp->builtins.no;
	OBJECT_INCREF(interp, res);
	return res;
}

bool boolobject_initoparrays(struct Interpreter *interp)
{
	return functionobject_add2array(interp, interp->oparrays.eq, "bool_eq", eq);
}
