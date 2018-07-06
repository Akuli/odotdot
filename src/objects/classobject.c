#include "classobject.h"
#include <assert.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include "../attribute.h"
#include "../check.h"
#include "../interpreter.h"
#include "../method.h"
#include "../objectsystem.h"
#include "../unicode.h"
#include "../utf8.h"
#include "array.h"
#include "errors.h"
#include "mapping.h"
#include "null.h"
#include "string.h"

static void class_foreachref(void *data, object_foreachrefcb cb, void *cbdata)
{
	struct ClassObjectData *casteddata = data;
	if (casteddata->baseclass) cb(casteddata->baseclass, cbdata);
	if (casteddata->setters) cb(casteddata->setters, cbdata);
	if (casteddata->getters) cb(casteddata->getters, cbdata);
}

static void class_destructor(void *data)
{
	free(((struct ClassObjectData *)data)->name.val);    // free(NULL) does nothing
	free(data);
}


static struct ClassObjectData *create_data(struct Interpreter *interp, struct Object *baseclass)
{
	struct ClassObjectData *data = malloc(sizeof(struct ClassObjectData));
	if (!data)
		return NULL;

	data->name.len = 0;
	data->name.val = NULL;
	data->baseclass = baseclass;
	if (baseclass) {
		OBJECT_INCREF(interp, baseclass);
		data->newinstance = ((struct ClassObjectData *) baseclass->objdata.data)->newinstance;   // may be NULL
	} else
		data->newinstance = NULL;
	data->setters = NULL;
	data->getters = NULL;
	return data;
}

// only for error handling, class_destructor() handles other cases
static void free_data(struct Interpreter *interp, struct ClassObjectData *data)
{
	if (data->baseclass)
		OBJECT_DECREF(interp, data->baseclass);
	free(data);
}

struct Object *classobject_new_noerr(struct Interpreter *interp, struct Object *baseclass, struct Object* (*newinstance)(struct Interpreter *, struct Object *args, struct Object *opts))
{
	struct ClassObjectData *data = create_data(interp, baseclass);
	if (!data)
		return NULL;

	if (newinstance)
		data->newinstance = newinstance;
	// else, use the baseclass newinstance, see create_data

	// interp->Class can be NULL, see builtins_setup()
	struct Object *klass = object_new_noerr(interp, interp->builtins.Class, (struct ObjectData){.data=data, .foreachref=class_foreachref, .destructor=class_destructor});
	if (!klass) {
		free_data(interp, data);
		return NULL;
	}
	return klass;
}

bool classobject_setname(struct Interpreter *interp, struct Object *klass, char *name)
{
	struct ClassObjectData *data = klass->objdata.data;

	void *maybegonnafree = data->name.val;
	bool ok = utf8_decode(interp, name, strlen(name), &data->name);
	if (ok)
		free(maybegonnafree);    // free(NULL) does nothing
	return ok;
}

struct Object *classobject_new(struct Interpreter *interp, char *name, struct Object *baseclass, struct Object* (*newinstance)(struct Interpreter *, struct Object *args, struct Object *opts))
{
	assert(interp->builtins.Class);
	assert(interp->builtins.nomemerr);

	struct Object *klass = classobject_new_noerr(interp, baseclass, newinstance);
	if (!klass) {
		errorobject_thrownomem(interp);
		return NULL;
	}

	if (!classobject_setname(interp, klass, name)) {
		OBJECT_DECREF(interp, klass);
		return NULL;
	}
	return klass;
}


bool classobject_issubclassof(struct Object *sub, struct Object *super)
{
	do {
		if (sub == super)
			return true;
	} while ((sub = ((struct ClassObjectData*) sub->objdata.data)->baseclass));
	return false;
}


static struct Object *class_newinstance(struct Interpreter *interp, struct Object *args, struct Object *opts)
{
	if (!check_args(interp, args, interp->builtins.Class, interp->builtins.String, interp->builtins.Class, NULL)) return NULL;
	if (!check_no_opts(interp, opts)) return NULL;
	struct Object *classclass = ARRAYOBJECT_GET(args, 0);
	struct Object *name = ARRAYOBJECT_GET(args, 1);
	struct Object *baseclass = ARRAYOBJECT_GET(args, 2);

	struct ClassObjectData *data = create_data(interp, baseclass);
	if (!data) {
		errorobject_thrownomem(interp);
		return NULL;
	}

	assert(!data->name.val);   // shouldn't need a free() here
	if (!unicodestring_copyinto(interp, *((struct UnicodeString *) name->objdata.data), &data->name)) {
		free_data(interp, data);
		return NULL;
	}

	struct Object *klass = object_new_noerr(interp, classclass, (struct ObjectData){.data=data, .foreachref=class_foreachref, .destructor=class_destructor});
	if (!klass) {
		free(data->name.val);
		free_data(interp, data);
		errorobject_thrownomem(interp);
		return NULL;
	}
	return klass;
}

struct Object *classobject_create_Class_noerr(struct Interpreter *interp)
{
	return classobject_new_noerr(interp, interp->builtins.Object, class_newinstance);
}


// override Object's setup to allow arguments
static struct Object *setup(struct Interpreter *interp, struct ObjectData nulldata, struct Object *args, struct Object *opts) {
	return nullobject_get(interp);
}

static struct Object *name_getter(struct Interpreter *interp, struct ObjectData nulldata, struct Object *args, struct Object *opts)
{
	if (!check_args(interp, args, interp->builtins.Class, NULL)) return NULL;
	if (!check_no_opts(interp, opts)) return NULL;

	// allocating more memory in newfromustr_copy is not a problem
	// performance-critical stuff doesn't access class names in a tight loop
	// unless something's wrong
	struct ClassObjectData *data = ARRAYOBJECT_GET(args, 0)->objdata.data;
	return stringobject_newfromustr_copy(interp, data->name);
}

static struct Object *baseclass_getter(struct Interpreter *interp, struct ObjectData nulldata, struct Object *args, struct Object *opts)
{
	if (!check_args(interp, args, interp->builtins.Class, NULL)) return NULL;
	if (!check_no_opts(interp, opts)) return NULL;

	struct ClassObjectData *data = ARRAYOBJECT_GET(args, 0)->objdata.data;

	if (!data->baseclass) {
		// it's Object
		// it could be some other class too, but only Object SHOULD have this....
		return nullobject_get(interp);
	}
	OBJECT_INCREF(interp, data->baseclass);
	return data->baseclass;
}

static struct Object *getters_getter(struct Interpreter *interp, struct ObjectData nulldata, struct Object *args, struct Object *opts)
{
	if (!check_args(interp, args, interp->builtins.Class, NULL)) return NULL;
	if (!check_no_opts(interp, opts)) return NULL;

	struct ClassObjectData *data = ARRAYOBJECT_GET(args, 0)->objdata.data;
	if (!data->getters) {
		if (!(data->getters = mappingobject_newempty(interp)))
			return NULL;
	}

	OBJECT_INCREF(interp, data->getters);
	return data->getters;
}

static struct Object *setters_getter(struct Interpreter *interp, struct ObjectData nulldata, struct Object *args, struct Object *opts)
{
	if (!check_args(interp, args, interp->builtins.Class, NULL)) return NULL;
	if (!check_no_opts(interp, opts)) return NULL;

	struct ClassObjectData *data = ARRAYOBJECT_GET(args, 0)->objdata.data;
	if (!data->setters) {
		if (!(data->setters = mappingobject_newempty(interp)))
			return NULL;
	}

	OBJECT_INCREF(interp, data->setters);
	return data->setters;
}

bool classobject_addmethods(struct Interpreter *interp)
{
	if (!method_add(interp, interp->builtins.Class, "setup", setup)) return false;
	if (!attribute_add(interp, interp->builtins.Class, "name", name_getter, NULL)) return false;
	if (!attribute_add(interp, interp->builtins.Class, "baseclass", baseclass_getter, NULL)) return false;
	if (!attribute_add(interp, interp->builtins.Class, "getters", getters_getter, NULL)) return false;
	if (!attribute_add(interp, interp->builtins.Class, "setters", setters_getter, NULL)) return false;
	return true;
}
