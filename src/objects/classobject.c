#include "classobject.h"
#include <assert.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include "../attribute.h"
#include "../check.h"
#include "../interpreter.h"
#include "../method.h"
#include "../objectsystem.h"
#include "../unicode.h"
#include "array.h"
#include "errors.h"
#include "mapping.h"
#include "null.h"
#include "string.h"

static void class_destructor(struct Object *klass)
{
	// class_foreachref takes care of most other things
	struct ClassObjectData *data = klass->data;
	if (data) {
		free(data->name.val);    // free(NULL) does nothing
		free(data);
	}
}


static struct ClassObjectData *create_data(struct Interpreter *interp, struct Object *baseclass, void (*foreachref)(struct Object*, void*, classobject_foreachrefcb))
{
	struct ClassObjectData *data = malloc(sizeof(struct ClassObjectData));
	if (!data)
		return NULL;

	data->name.len = 0;
	data->name.val = NULL;
	data->baseclass = baseclass;
	if (baseclass)
		OBJECT_INCREF(interp, baseclass);
	data->setters = NULL;
	data->getters = NULL;
	data->foreachref = foreachref;
	return data;
}

// only for error handling, class_destructor() handles other cases
static void free_data(struct Interpreter *interp, struct ClassObjectData *data)
{
	if (data->baseclass)
		OBJECT_DECREF(interp, data->baseclass);
	free(data);
}

struct Object *classobject_new_noerr(struct Interpreter *interp, char *name, struct Object *baseclass, void (*foreachref)(struct Object*, void*, classobject_foreachrefcb))
{
	struct ClassObjectData *data = create_data(interp, baseclass, foreachref);
	if (!data)
		return NULL;

	// interp->Class can be NULL, see builtins_setup()
	struct Object *klass = object_new_noerr(interp, interp->builtins.Class, data, class_destructor);
	if (!klass) {
		free_data(interp, data);
		return NULL;
	}
	return klass;
}

bool classobject_setname(struct Interpreter *interp, struct Object *klass, char *name)
{
	struct ClassObjectData *data = klass->data;

	// if utf8_decode fails, the name must be left untouched
	void *maybegonnafree = data->name.val;
	bool ok = utf8_decode(interp, name, strlen(name), &data->name);
	if (ok)
		free(maybegonnafree);    // free(NULL) does nothing
	return ok;
}

struct Object *classobject_new(struct Interpreter *interp, char *name, struct Object *baseclass, void (*foreachref)(struct Object*, void*, classobject_foreachrefcb))
{
	assert(interp->builtins.Class);
	assert(interp->builtins.nomemerr);

	if (!classobject_isinstanceof(baseclass->klass, interp->builtins.Class)) {
		// TODO: test this
		errorobject_setwithfmt(interp, "cannot inherit a new class from %D", baseclass);
		return NULL;
	}

	struct Object *klass = classobject_new_noerr(interp, name, baseclass, foreachref);
	if (!klass) {
		errorobject_setnomem(interp);
		return NULL;
	}

	if (!classobject_setname(interp, klass, name)) {
		OBJECT_DECREF(interp, klass);
		return NULL;
	}
	return klass;
}


// TODO: get rid of this? it seems quite dumb
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

bool classobject_isinstanceof(struct Object *obj, struct Object *klass)
{
	struct Object *klass2 = obj->klass;
	do {
		if (klass2 == klass)
			return true;
	} while ((klass2 = ((struct ClassObjectData*) klass2->data)->baseclass));
	return false;
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


static struct Object *setup(struct Interpreter *interp, struct Object *args)
{
	if (!check_args(interp, args, interp->builtins.Class, interp->builtins.String, interp->builtins.Class, NULL))
		return NULL;
	struct Object *klass = ARRAYOBJECT_GET(args, 0);
	struct Object *name = ARRAYOBJECT_GET(args, 1);
	struct Object *baseclass = ARRAYOBJECT_GET(args, 2);

	if (klass->data) {
		errorobject_setwithfmt(interp, "setup was called twice");
		return NULL;
	}

	struct ClassObjectData *data = create_data(interp, baseclass, NULL);
	if (!data) {
		errorobject_setnomem(interp);
		return NULL;
	}

	assert(!data->name.val);   // shouldn't need a free() here
	if (!unicodestring_copyinto(interp, *((struct UnicodeString *) name->data), &data->name)) {
		free_data(interp, data);
		return NULL;
	}

	klass->data = data;
	klass->destructor = class_destructor;

	return nullobject_get(interp);
}

static struct Object *name_getter(struct Interpreter *interp, struct Object *args)
{
	if (!check_args(interp, args, interp->builtins.Class, NULL))
		return NULL;

	struct ClassObjectData *data = ARRAYOBJECT_GET(args, 0)->data;
	return stringobject_newfromustr(interp, data->name);
}

static struct Object *baseclass_getter(struct Interpreter *interp, struct Object *args)
{
	if (!check_args(interp, args, interp->builtins.Class, NULL))
		return NULL;

	struct ClassObjectData *data = ARRAYOBJECT_GET(args, 0)->data;

	if (!data->baseclass) {
		// it's Object
		// it could be some other class too, but only Object SHOULD have this....
		return nullobject_get(interp);
	}
	OBJECT_INCREF(interp, data->baseclass);
	return data->baseclass;
}

static struct Object *getters_getter(struct Interpreter *interp, struct Object *args)
{
	if (!check_args(interp, args, interp->builtins.Class, NULL))
		return NULL;

	struct ClassObjectData *data = ARRAYOBJECT_GET(args, 0)->data;
	if (!data->getters) {
		if (!(data->getters = mappingobject_newempty(interp)))
			return NULL;
	}

	OBJECT_INCREF(interp, data->getters);
	return data->getters;
}

static struct Object *setters_getter(struct Interpreter *interp, struct Object *args)
{
	if (!check_args(interp, args, interp->builtins.Class, NULL))
		return NULL;

	struct ClassObjectData *data = ARRAYOBJECT_GET(args, 0)->data;
	if (!data->setters) {
		if (!(data->setters = mappingobject_newempty(interp)))
			return NULL;
	}

	OBJECT_INCREF(interp, data->setters);
	return data->setters;
}

static struct Object *to_debug_string(struct Interpreter *interp, struct Object *args)
{
	if (!check_args(interp, args, interp->builtins.Class, NULL))
		return NULL;
	struct ClassObjectData *data = ARRAYOBJECT_GET(args, 0)->data;
	return stringobject_newfromfmt(interp, "<Class \"%U\">", data->name);
}

bool classobject_addmethods(struct Interpreter *interp)
{
	if (!method_add(interp, interp->builtins.Class, "setup", setup)) return false;
	if (!method_add(interp, interp->builtins.Class, "to_debug_string", to_debug_string)) return false;
	if (!attribute_add(interp, interp->builtins.Class, "name", name_getter, NULL)) return false;
	if (!attribute_add(interp, interp->builtins.Class, "baseclass", baseclass_getter, NULL)) return false;
	if (!attribute_add(interp, interp->builtins.Class, "getters", getters_getter, NULL)) return false;
	if (!attribute_add(interp, interp->builtins.Class, "setters", setters_getter, NULL)) return false;
	return true;
}
