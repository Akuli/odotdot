#include "null.h"
#include "../check.h"
#include "../interpreter.h"
#include "../method.h"
#include "../objectsystem.h"
#include "classobject.h"
#include "errors.h"
#include "string.h"

static struct Object *setup(struct Interpreter *interp, struct Object *args, struct Object *opts)
{
	errorobject_setwithfmt(interp, "new null objects cannot be created");
	return NULL;
}

static struct Object *to_debug_string(struct Interpreter *interp, struct Object *args, struct Object *opts)
{
	if (!check_args(interp, args, interp->builtins.null->klass, NULL)) return NULL;
	if (!check_no_opts(interp, opts)) return NULL;
	return stringobject_newfromcharptr(interp, "null");
}

struct Object *nullobject_create(struct Interpreter *interp)
{
	struct Object *klass = classobject_new(interp, "Null", interp->builtins.Object, NULL);
	if (!klass)
		return NULL;

	if (!method_add(interp, klass, "setup", setup)) goto error;
	if (!method_add(interp, klass, "to_debug_string", to_debug_string)) goto error;

	struct Object *nullobj = classobject_newinstance(interp, klass, NULL, NULL);
	OBJECT_DECREF(interp, klass);
	return nullobj;

error:
	OBJECT_DECREF(interp, klass);
	return NULL;
}

struct Object *nullobject_get(struct Interpreter *interp)
{
	OBJECT_INCREF(interp, interp->builtins.null);
	return interp->builtins.null;
}
