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

struct Object *nullobject_get(struct Interpreter *interp)
{
	OBJECT_INCREF(interp, interp->builtins.null);
	return interp->builtins.null;
}
