#include "object.h"
#include <stddef.h>
#include "../check.h"
#include "../interpreter.h"
#include "../method.h"
#include "../objectsystem.h"
#include "array.h"
#include "classobject.h"
#include "errors.h"
#include "function.h"
#include "integer.h"
#include "null.h"
#include "string.h"

static void object_foreachref(struct Object *obj, void *data, classobject_foreachrefcb cb)
{
	if (obj->klass)
		cb(obj->klass, data);
	if (obj->attrdata)
		cb(obj->attrdata, data);
}

struct Object *objectobject_createclass_noerr(struct Interpreter *interp)
{
	return classobject_new_noerr(interp, "Object", NULL, object_foreachref);
}


// setup does nothing by default
static struct Object *setup(struct Interpreter *interp, struct Object *argarr) {
	if (!check_args(interp, argarr, interp->builtins.Object, NULL)) return NULL;
	return nullobject_get(interp);
}

static struct Object *to_debug_string(struct Interpreter *interp, struct Object *argarr)
{
	if (ARRAYOBJECT_LEN(argarr) != 1) {
		errorobject_setwithfmt(interp, "Object.to_debug_string takes exactly 1 argument");
		return NULL;
	}
	return method_call(interp, ARRAYOBJECT_GET(argarr, 0), "to_string", NULL);
}

bool objectobject_addmethods(struct Interpreter *interp)
{
	if (method_add(interp, interp->builtins.Object, "setup", setup) == false) return false;
	if (method_add(interp, interp->builtins.Object, "to_debug_string", to_debug_string) == false) return false;
	return true;
}
