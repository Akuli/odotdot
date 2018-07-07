#include "bool.h"
#include <assert.h>
#include <stdbool.h>
#include <stddef.h>
#include "../interpreter.h"
#include "../objectsystem.h"
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

	struct Object *yay = object_new_noerr(interp, klass, (struct ObjectData){.data=NULL, .foreachref=NULL, .destructor=NULL});
	if (!yay) {
		OBJECT_DECREF(interp, klass);
		errorobject_thrownomem(interp);
		return false;
	}

	struct Object *nay = object_new_noerr(interp, klass, (struct ObjectData){.data=NULL, .foreachref=NULL, .destructor=NULL});
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

struct Object *boolobject_get(struct Interpreter *interp, bool b)
{
	struct Object *res = b? interp->builtins.yes : interp->builtins.no;
	OBJECT_INCREF(interp, res);
	return res;
}
