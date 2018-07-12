#include "object.h"
#include <stddef.h>
#include "../check.h"
#include "../interpreter.h"
#include "../method.h"
#include "../objectsystem.h"
#include "classobject.h"
#include "string.h"

struct Object *objectobject_createclass_noerr(struct Interpreter *interp)
{
	return classobject_new_noerr(interp, NULL, NULL);
}


// setup does nothing by default
static bool setup(struct Interpreter *interp, struct ObjectData thisdata, struct Object *args, struct Object *opts) {
	if (!check_args(interp, args, NULL)) return false;
	if (!check_no_opts(interp, opts)) return false;
	return true;
}

static struct Object *to_debug_string(struct Interpreter *interp, struct ObjectData thisdata, struct Object *args, struct Object *opts)
{
	check_args(interp, args, NULL);
	check_no_opts(interp, opts);

	struct Object *obj = thisdata.data;
	struct ClassObjectData *classdata = obj->klass->objdata.data;
	return stringobject_newfromfmt(interp, "<%U at %p>", classdata->name, (void*)obj);
}

bool objectobject_addmethods(struct Interpreter *interp)
{
	if (!method_add_noret(interp, interp->builtins.Object, "setup", setup)) return false;
	if (!method_add_yesret(interp, interp->builtins.Object, "to_debug_string", to_debug_string)) return false;
	return true;
}
