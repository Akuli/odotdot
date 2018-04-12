#include "object.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include "string.h"
#include "../common.h"
#include "../interpreter.h"
#include "../method.h"
#include "../objectsystem.h"


struct ObjectClassInfo *objectobject_createclass(void)
{
	return objectclassinfo_new("Object", NULL, NULL, NULL);
}


#define BIG_ENOUGH 50
static struct Object *to_string(struct Context *ctx, struct Object **errptr, struct Object **args, size_t nargs)
{
	if (functionobject_checktypes(ctx, errptr, args, nargs, "Object", NULL) == STATUS_ERROR)
		return NULL;

	char *name = ((struct ObjectClassInfo*) args[0]->klass->data)->name;
	// FIXME: unicode_iswovel is supposed to be used with unicodes, so this breaks with e.g. ä, ö
	uint32_t first = name[0];    // needed to suppress compiler warnings
	return stringobject_newfromfmt(ctx, errptr, "<%s %s at %p>", unicode_iswovel(first) ? "an" : "a", name, (void *) args[0]);
}

static struct Object *to_debug_string(struct Context *ctx, struct Object **errptr, struct Object **args, size_t nargs)
{
	if (functionobject_checktypes(ctx, errptr, args, nargs, "Object", NULL) == STATUS_ERROR)
		return NULL;
	return method_call(ctx, errptr, args[0], "to_string", NULL);
}

int objectobject_addmethods(struct Interpreter *interp, struct Object **errptr)
{
	struct Object *objectclass = interpreter_getbuiltin(interp, errptr, "Object");
	if (!objectclass)
		return STATUS_ERROR;

	if (method_add(interp, errptr, objectclass, "to_string", to_string) == STATUS_ERROR) goto error;
	if (method_add(interp, errptr, objectclass, "to_debug_string", to_debug_string) == STATUS_ERROR) goto error;

	OBJECT_DECREF(interp, objectclass);
	return STATUS_OK;

error:
	OBJECT_DECREF(interp, objectclass);
	return STATUS_ERROR;
}
