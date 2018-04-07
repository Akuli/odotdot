#include "classobject.h"
#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include "../common.h"
#include "../interpreter.h"
#include "../objectsystem.h"

// FIXME: this is how i pass interp to classobject_free(), and it sucks
struct Interpreter *shit;

static void classobject_free(struct Object *obj)
{
	objectclassinfo_free(shit, obj->data);
}

int classobject_createclass(struct Interpreter *interp, struct Object **errptr, struct ObjectClassInfo *objectclass)
{
	shit = interp;
	struct ObjectClassInfo *info = objectclassinfo_new("Class", objectclass, NULL, classobject_free);
	if (!info) {
		*errptr = interp->nomemerr;
		return STATUS_ERROR;
	}

	struct Object *klass = object_new(interp, NULL, info);
	if (!klass) {
		objectclassinfo_free(interp, info);
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
		objectclassinfo_free(interp, info);
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

int classobject_addmethod(struct Interpreter *interp, struct Object **errptr, struct Object *klass, char *name, functionobject_cfunc cfunc, void *data)
{
	assert(klass->klass = interp->classclass);    // TODO: better type check

	struct UnicodeString *uname = malloc(sizeof(struct UnicodeString));
	if (!uname) {
		*errptr = interp->nomemerr;
		return STATUS_ERROR;
	}

	char errormsg[100];
	int status = utf8_decode(name, strlen(name), uname, errormsg);
	if (status != STATUS_OK) {
		assert(status == STATUS_NOMEM);
		*errptr = interp->nomemerr;
		free(uname);
		return STATUS_ERROR;
	}

	struct Object *func = functionobject_new(interp, errptr, cfunc, data);
	if (!func) {
		free(uname->val);
		free(uname);
		return STATUS_ERROR;
	}

	if (hashtable_set(((struct ObjectClassInfo *) klass->data)->methods, uname, unicodestring_hash(*uname), func, NULL) == STATUS_NOMEM) {
		*errptr = interp->nomemerr;
		free(uname->val);
		free(uname);
		OBJECT_DECREF(interp, func);
		return STATUS_ERROR;
	}

	// don't decref func, now info->methods holds the reference
	// don't free uname or uname->val because uname is now used as a key in info->methods
	return STATUS_OK;
}

struct Object *classobject_getmethod_ustr(struct Interpreter *interp, struct Object **errptr, struct Object *klass, struct UnicodeString uname)
{
	assert(klass->klass == interp->classclass);    // TODO: better type check

	struct Object *res;
	int found = hashtable_get(((struct ObjectClassInfo *) klass->data)->methods, &uname, unicodestring_hash(uname), (void **)(&res), NULL);
	if (!found)
		return NULL;
	OBJECT_INCREF(interp, res);
	return res;
}

struct Object *classobject_getmethod(struct Interpreter *interp, struct Object **errptr, struct Object *klass, char *name)
{
	assert(klass->klass == interp->classclass);    // TODO: better type check

	struct UnicodeString *uname = malloc(sizeof(struct UnicodeString));
	if (!uname) {
		*errptr = interp->nomemerr;
		return NULL;
	}

	char errormsg[100];
	int status = utf8_decode(name, strlen(name), uname, errormsg);
	if (status != STATUS_OK) {
		assert(status == STATUS_NOMEM);
		*errptr = interp->nomemerr;
		free(uname);
		return NULL;
	}

	struct Object *res = classobject_getmethod_ustr(interp, errptr, klass, *uname);
	free(uname->val);
	free(uname);
	return res;
}

