#include "classobject.h"
#include <assert.h>
#include <stdlib.h>
#include "../common.h"
#include "../interpreter.h"
#include "../objectsystem.h"

static void classobject_free(struct Object *obj)
{
	objectclassinfo_free(obj->data);
}

int classobject_createclass(struct Interpreter *interp, struct Object **errptr, struct ObjectClassInfo *objectclass)
{
	struct ObjectClassInfo *info = objectclassinfo_new("Class", objectclass, NULL, classobject_free);
	if (!info) {
		*errptr = interp->nomemerr;
		return STATUS_ERROR;
	}

	struct Object *klass = object_new(interp, NULL, info);
	if (!klass) {
		objectclassinfo_free(info);
		*errptr = interp->nomemerr;
		return STATUS_ERROR;
	}
	klass->klass = klass;      // cyclic reference!
	OBJECT_INCREF(interp, klass);   // FIXME: is this good?
	interp->classclass = klass;
	return STATUS_OK;
}

struct Object *classobject_new(struct Interpreter *interp, struct Object **errptr, char *name, struct Object *base, objectclassinfo_foreachref foreachref, void (*destructor)(struct Object *))
{
	// TODO: better type check
	assert(base->klass == interp->classclass);

	struct ObjectClassInfo *info = objectclassinfo_new(name, base->data, foreachref, destructor);
	if (!info) {
		*errptr = interp->nomemerr;
		return NULL;
	}

	struct Object *klass = classobject_newinstance(interp, errptr, interp->classclass, info);
	if (!klass) {
		objectclassinfo_free(info);
		return NULL;
	}
	return klass;
}

struct Object *classobject_newinstance(struct Interpreter *interp, struct Object **errptr, struct Object *klass, void *data)
{
	assert(klass->klass == interp->classclass);      // TODO: better type check
	struct Object *instance = object_new(interp, klass, data);
	if (!instance) {
		*errptr = interp->nomemerr;
		return NULL;
	}
	OBJECT_INCREF(interp, klass);
	return instance;
}

struct Object *classobject_newfromclassinfo(struct Interpreter *interp, struct Object **errptr, struct ObjectClassInfo *wrapped)
{
	return classobject_newinstance(interp, errptr, interp->classclass, wrapped);
}
