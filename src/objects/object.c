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

struct Object *objectobject_createclass_noerr(struct Interpreter *interp)
{
	return classobject_new_noerr(interp, NULL, true);
}


// setup does nothing by default
static struct Object *setup(struct Interpreter *interp, struct Object *args, struct Object *opts) {
	if (!check_args(interp, args, interp->builtins.Object, NULL)) return NULL;
	if (!check_no_opts(interp, opts)) return NULL;
	return nullobject_get(interp);
}

static struct Object *to_debug_string(struct Interpreter *interp, struct Object *args, struct Object *opts)
{
	check_args(interp, args, interp->builtins.Object, NULL);
	check_no_opts(interp, opts);

	struct Object *obj = ARRAYOBJECT_GET(args, 0);
	struct ClassObjectData *classdata = obj->klass->data;
	return stringobject_newfromfmt(interp, "<%U at %p>", classdata->name, (void*)obj);
}

bool objectobject_addmethods(struct Interpreter *interp)
{
	if (!method_add(interp, interp->builtins.Object, "setup", setup)) return false;
	if (!method_add(interp, interp->builtins.Object, "to_debug_string", to_debug_string)) return false;
	return true;
}
