#include "object.h"
#include <stddef.h>
#include "classobject.h"
#include "function.h"
#include "string.h"
#include "../common.h"
#include "../hashtable.h"
#include "../interpreter.h"
#include "../method.h"
#include "../objectsystem.h"

static void object_foreachref(struct Object *obj, void *data, classobject_foreachrefcb cb)
{
	if (obj->klass)
		cb(obj->klass, data);
	if (obj->attrs) {
		struct HashTableIterator iter;
		hashtable_iterbegin(obj->attrs, &iter);
		while (hashtable_iternext(&iter))
			cb(iter.value, data);
	}
}

struct Object *objectobject_createclass(struct Interpreter *interp)
{
	return classobject_new_noerrptr(interp, "Object", NULL, 0, object_foreachref, NULL);
}


static struct Object *to_string(struct Context *ctx, struct Object **errptr, struct Object **args, size_t nargs)
{
	if (functionobject_checktypes(ctx, errptr, args, nargs, "Object", NULL) == STATUS_ERROR)
		return NULL;

	char *name = ((struct ClassObjectData*) args[0]->klass->data)->name;
	return stringobject_newfromfmt(ctx, errptr, "<%s at %p>", name, (void *) args[0]);
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
