#include "option.h"
#include <assert.h>
#include <stddef.h>
#include "../attribute.h"
#include "../check.h"
#include "../interpreter.h"
#include "../method.h"
#include "../objectsystem.h"
#include "array.h"
#include "bool.h"
#include "classobject.h"
#include "errors.h"

// none is represented with NULL data, otherwise the data is the value
// but none's foreachref is also set to NULL, so no need to check for that here
void option_foreachref(void *data, object_foreachrefcb cb, void *cbdata)
{
	cb((struct Object*)data, cbdata);
}

static struct Object *newinstance(struct Interpreter *interp, struct Object *args, struct Object *opts)
{
	if (!check_args(interp, args, interp->builtins.Class, interp->builtins.Object, NULL)) return NULL;
	if (!check_no_opts(interp, opts)) return NULL;
	struct Object *val = ARRAYOBJECT_GET(args, 1);
	assert(val);   // none is not created with this

	struct Object *option = object_new_noerr(interp, ARRAYOBJECT_GET(args, 0), (struct ObjectData){.data=val, .foreachref=option_foreachref, .destructor=NULL});
	if (!option) {
		errorobject_thrownomem(interp);
		return NULL;
	}
	OBJECT_INCREF(interp, val);     // for the option's data

	return option;
}

struct Object *optionobject_createclass_noerr(struct Interpreter *interp)
{
	assert(interp->builtins.Object);
	return classobject_new_noerr(interp, interp->builtins.Object, newinstance);
}

struct Object *optionobject_createnone_noerr(struct Interpreter *interp)
{
	return object_new_noerr(interp, interp->builtins.Option, (struct ObjectData){.data=NULL, .foreachref=NULL, .destructor=NULL});
}


// newinstance does everything, this does nothing just to allow passing arguments handled by newinstance
static bool setup(struct Interpreter *interp, struct ObjectData thisdata, struct Object *args, struct Object *opts) {
	return true;
}

static struct Object *get_value(struct Interpreter *interp, struct ObjectData thisdata, struct Object *args, struct Object *opts)
{
	if (!check_args(interp, args, NULL)) return NULL;
	if (!check_no_opts(interp, opts)) return NULL;

	struct Object *option = thisdata.data;
	struct Object *val = option->objdata.data;
	if (!val) {
		errorobject_throwfmt(interp, "ValueError", "cannot get the value of none");
		return NULL;
	}
	OBJECT_INCREF(interp, val);
	return val;
}

bool optionobject_addmethods(struct Interpreter *interp)
{
	if (!method_add_noret(interp, interp->builtins.Option, "setup", setup)) return false;
	if (!method_add_yesret(interp, interp->builtins.Option, "get_value", get_value)) return false;
	return true;
}


struct Object *optionobject_new(struct Interpreter *interp, struct Object *val)
{
	struct Object *option = object_new_noerr(interp, interp->builtins.Option, (struct ObjectData){.data=val, .foreachref=option_foreachref, .destructor=NULL});
	if (!option) {
		errorobject_thrownomem(interp);
		return NULL;
	}
	OBJECT_INCREF(interp, val);     // for the option's data
	return option;
}
