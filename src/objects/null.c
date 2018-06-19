#include "null.h"
#include <assert.h>
#include <stdbool.h>
#include "../check.h"
#include "../interpreter.h"
#include "../method.h"
#include "../objectsystem.h"
#include "classobject.h"
#include "errors.h"
#include "string.h"

static struct Object *newinstance(struct Interpreter *interp, struct Object *args, struct Object *opts)
{
	errorobject_throwfmt(interp, "TypeError", "new null objects cannot be created");
	return NULL;
}

static struct Object *to_debug_string(struct Interpreter *interp, struct Object *args, struct Object *opts)
{
	if (!check_args(interp, args, interp->builtins.null->klass, NULL)) return NULL;
	if (!check_no_opts(interp, opts)) return NULL;
	return stringobject_newfromcharptr(interp, "null");
}

struct Object *nullobject_create_noerr(struct Interpreter *interp)
{
	assert(interp->builtins.Object);
	struct Object *klass = classobject_new_noerr(interp, interp->builtins.Object, newinstance);
	if (!klass)
		return NULL;

	struct Object *nullobj = object_new_noerr(interp, klass, NULL, NULL, NULL);
	OBJECT_DECREF(interp, klass);
	return nullobj;
}

bool nullobject_addmethods(struct Interpreter *interp)
{
	if (!method_add(interp, interp->builtins.null->klass, "to_debug_string", to_debug_string)) return false;
	return true;
}


struct Object *nullobject_get(struct Interpreter *interp)
{
	OBJECT_INCREF(interp, interp->builtins.null);
	return interp->builtins.null;
}
