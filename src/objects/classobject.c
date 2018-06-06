#include "classobject.h"
#include <assert.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include "../check.h"
#include "../common.h"
#include "../interpreter.h"
#include "../method.h"
#include "../objectsystem.h"
#include "../unicode.h"
#include "errors.h"
#include "mapping.h"

static void class_destructor(struct Object *klass)
{
	// class_foreachref takes care of most other things
	free(klass->data);
}


struct Object *classobject_new_noerr(struct Interpreter *interp, char *name, struct Object *base, void (*foreachref)(struct Object*, void*, classobject_foreachrefcb))
{
	struct ClassObjectData *data = malloc(sizeof(struct ClassObjectData));
	if (!data)
		return NULL;

	strncpy(data->name, name, 10);
	data->name[9] = 0;
	data->baseclass = base;
	if (base)
		OBJECT_INCREF(interp, base);
	data->setters = NULL;
	data->getters = NULL;
	data->foreachref = foreachref;

	// interp->Class can be NULL, see builtins_setup()
	struct Object *klass = object_new_noerr(interp, interp->builtins.Class, data, class_destructor);
	if (!klass) {
		OBJECT_DECREF(interp, base);
		free(data);
		return NULL;
	}

	return klass;
}

struct Object *classobject_new(struct Interpreter *interp, char *name, struct Object *base, void (*foreachref)(struct Object*, void*, classobject_foreachrefcb))
{
	assert(interp->builtins.Class);
	assert(interp->builtins.nomemerr);

	if (!classobject_isinstanceof(base->klass, interp->builtins.Class)) {
		// TODO: test this
		errorobject_setwithfmt(interp, "cannot inherit a new class from %D", base);
		return NULL;
	}

	struct Object *klass = classobject_new_noerr(interp, name, base, foreachref);
	if (!klass) {
		errorobject_setnomem(interp);
		return NULL;
	}

	return klass;
}

struct Object *classobject_newinstance(struct Interpreter *interp, struct Object *klass, void *data, void (*destructor)(struct Object*))
{
	if (!classobject_isinstanceof(klass, interp->builtins.Class)) {
		// TODO: test this
		errorobject_setwithfmt(interp, "cannot create an instance of %D", klass);
		return NULL;
	}
	struct Object *instance = object_new_noerr(interp, klass, data, destructor);
	if (!instance) {
		errorobject_setnomem(interp);
		return NULL;
	}
	return instance;
}

int classobject_isinstanceof(struct Object *obj, struct Object *klass)
{
	struct Object *klass2 = obj->klass;
	do {
		if (klass2 == klass)
			return 1;
	} while ((klass2 = ((struct ClassObjectData*) klass2->data)->baseclass));
	return 0;
}

void classobject_runforeachref(struct Object *obj, void *data, classobject_foreachrefcb cb)
{
	// this must not fail if obj->klass is NULL because builtins_setup() is lol
	struct Object *klass = obj->klass;
	while (klass) {
		struct ClassObjectData *klassdata = klass->data;
		if (klassdata->foreachref)
			klassdata->foreachref(obj, data, cb);
		klass = klassdata->baseclass;
	}
}


static void class_foreachref(struct Object *klass, void *cbdata, classobject_foreachrefcb cb)
{
	struct ClassObjectData *data = klass->data;
	if (data->baseclass) cb(data->baseclass, cbdata);
	if (data->setters) cb(data->setters, cbdata);
	if (data->getters) cb(data->getters, cbdata);
}


struct Object *classobject_create_Class_noerr(struct Interpreter *interp)
{
	// TODO: should the name of class objects be implemented as an attribute?
	return classobject_new_noerr(interp, "Class", interp->builtins.Object, class_foreachref);
}

static struct Object *setup(struct Interpreter *interp, struct Object *argarr)
{
	errorobject_setwithfmt(interp, "classes can't be created with (new Class) yet");
	return NULL;
}

int classobject_addmethods(struct Interpreter *interp)
{
	if (method_add(interp, interp->builtins.Class, "setup", setup) == STATUS_ERROR) return STATUS_ERROR;
	// TODO: to_debug_string
	return STATUS_OK;
}
