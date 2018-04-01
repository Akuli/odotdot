#include "assert.h"
#include "classobject.h"
#include "../common.h"
#include "../interpreter.h"
#include "../objectsystem.h"

static void classobject_free(struct Object *obj)
{
	// TODO: objectclassinfo_free(obj->data);  ???
}

struct ObjectClassInfo *classobject_createclass(struct Interpreter *interp, struct Object **errptr, struct ObjectClassInfo *objectclass)
{
	struct ObjectClassInfo *res = objectclassinfo_new(objectclass, NULL, classobject_free);
	if (!res) {
		*errptr = interp->nomemerr;
		// TODO: decref objectclass or something?
		return NULL;
	}
	return res;
}

struct Object *classobject_new(struct Interpreter *interp, struct Object **errptr, struct Object *base, objectclassinfo_foreachref foreachref, void (*destructor)(struct Object *))
{
	// TODO: better type check
	assert(base->klass == interp->classobjectinfo);

	struct ObjectClassInfo *info = objectclassinfo_new((struct ObjectClassInfo*) base->data, foreachref, destructor);
	if (!info) {
		*errptr = interp->nomemerr;
		return NULL;
	}

	struct Object *klass = classobject_newfromclassinfo(interp, errptr, info);
	if (!klass) {
		objectclassinfo_free(info);
		return NULL;
	}

	return klass;
}

struct Object *classobject_newfromclassinfo(struct Interpreter *interp, struct Object **errptr, struct ObjectClassInfo *wrapped)
{
	assert(interp->classobjectinfo);
	struct Object *klass = object_new(interp->classobjectinfo);
	if (!klass) {
		*errptr = interp->nomemerr;
		return NULL;
	}
	klass->data = wrapped;
	return klass;
}
