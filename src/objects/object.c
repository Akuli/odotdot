#include "object.h"
#include <stddef.h>
#include "classobject.h"
#include "errors.h"
#include "function.h"
#include "integer.h"
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
	return classobject_new_noerrptr(interp, "Object", NULL, 0, object_foreachref);
}


static struct Object *to_string(struct Interpreter *interp, struct Object **errptr, struct Object **args, size_t nargs)
{
	// functionobject_checktypes may call to_string when creating an error message
	// so we can't use it here, otherwise this may recurse
	if (nargs != 1) {
		errorobject_setwithfmt(interp, errptr, "Object::to_string takes exactly 1 argument");
		return NULL;
	}

	char *name = ((struct ClassObjectData*) args[0]->klass->data)->name;
	return stringobject_newfromfmt(interp, errptr, "<%s at %p>", name, (void *) args[0]);
}

static struct Object *to_debug_string(struct Interpreter *interp, struct Object **errptr, struct Object **args, size_t nargs)
{
	if (nargs != 1) {
		errorobject_setwithfmt(interp, errptr, "Object::to_debug_string takes exactly 1 argument");
		return NULL;
	}
	return method_call(interp, errptr, args[0], "to_string", NULL);
}

static struct Object *get_hash_value(struct Interpreter *interp, struct Object **errptr, struct Object **args, size_t nargs)
{
	if (functionobject_checktypes(interp, errptr, args, nargs, "Object", NULL) == STATUS_ERROR)
		return NULL;

	long long value = (unsigned int)((uintptr_t) args[0]);
	// TODO: add more randomness?
	return integerobject_newfromlonglong(interp, errptr, value);
}

int objectobject_addmethods(struct Interpreter *interp, struct Object **errptr)
{
	struct Object *objectclass = interpreter_getbuiltin(interp, errptr, "Object");
	if (!objectclass)
		return STATUS_ERROR;

	if (method_add(interp, errptr, objectclass, "to_string", to_string) == STATUS_ERROR) goto error;
	if (method_add(interp, errptr, objectclass, "to_debug_string", to_debug_string) == STATUS_ERROR) goto error;
	if (method_add(interp, errptr, objectclass, "get_hash_value", get_hash_value) == STATUS_ERROR) goto error;

	OBJECT_DECREF(interp, objectclass);
	return STATUS_OK;

error:
	OBJECT_DECREF(interp, objectclass);
	return STATUS_ERROR;
}
