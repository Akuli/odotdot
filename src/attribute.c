// TODO: test these, or just write a lot of code that uses them and test that
#include "attribute.h"
#include <assert.h>
#include <stdbool.h>
#include "common.h"
#include "method.h"
#include "unicode.h"
#include "objects/classobject.h"
#include "objects/errors.h"
#include "objects/function.h"
#include "objects/mapping.h"
#include "objects/string.h"

// returns true on success and false on failure
static bool init_mappings(struct Interpreter *interp, struct ClassObjectData *data)
{
	if (!data->getters) {
		if (!(data->getters = mappingobject_newempty(interp)))
			return false;
	}
	if (!data->setters) {
		if (!(data->setters = mappingobject_newempty(interp)))
			return false;
	}
	return true;
}


int attribute_add(struct Interpreter *interp, struct Object *klass, char *name, functionobject_cfunc getter, functionobject_cfunc setter)
{
	assert(interp->builtins.mappingclass && interp->builtins.functionclass);   // must not be called tooo early by builtins_setup()
	assert(setter || getter);

	struct ClassObjectData *data = klass->data;
	if (!init_mappings(interp, data))
		return STATUS_ERROR;

	struct Object *string = stringobject_newfromcharptr(interp, name);
	if (!string)
		return STATUS_ERROR;

	// this is kinda copy/pasta, but... not 2 bad imo
	if (getter) {
		struct Object *getterobj = functionobject_new(interp, getter);
		if (!getterobj)
			goto error;
		if (mappingobject_set(interp, data->getters, string, getterobj) == STATUS_ERROR) {
			OBJECT_DECREF(interp, getterobj);
			goto error;
		}
		OBJECT_DECREF(interp, getterobj);
	}
	if (setter) {
		struct Object *setterobj = functionobject_new(interp, setter);
		if (!setterobj)
			goto error;
		if (mappingobject_set(interp, data->setters, string, setterobj) == STATUS_ERROR) {
			OBJECT_DECREF(interp, setterobj);
			goto error;
		}
		OBJECT_DECREF(interp, setterobj);
	}

	OBJECT_DECREF(interp, string);
	return STATUS_OK;

error:
	OBJECT_DECREF(interp, string);
	return STATUS_ERROR;
}


struct Object *attribute_getwithstringobj(struct Interpreter *interp, struct Object *obj, struct Object *stringobj)
{
	struct ClassObjectData *data = obj->klass->data;
	if (!init_mappings(interp, data))
		return NULL;

	struct Object *getter = mappingobject_get(interp, data->getters, stringobj);
	if (!getter) {
		// mappingobject_get is shit, so we need more shit here
		if (interp->err)
			return NULL;
		errorobject_setwithfmt(interp, "%s objects have no attribute named %D", data->name, stringobj);
		return NULL;
	}

	struct Object *res = functionobject_call(interp, getter, obj, NULL);
	OBJECT_DECREF(interp, getter);
	return res;
}

struct Object *attribute_get(struct Interpreter *interp, struct Object *obj, char *attr)
{
	struct Object *stringobj = stringobject_newfromcharptr(interp, attr);
	if (!stringobj)
		return NULL;

	struct Object *res = attribute_getwithstringobj(interp, obj, stringobj);
	OBJECT_DECREF(interp, stringobj);
	return res;
}


int attribute_setwithstringobj(struct Interpreter *interp, struct Object *obj, struct Object *stringobj, struct Object *val)
{
	struct ClassObjectData *data = obj->klass->data;
	if (!init_mappings(interp, data))
		return STATUS_ERROR;

	struct Object *setter = mappingobject_get(interp, data->setters, stringobj);
	if (!setter) {
		// mappingobject_get is shit, so we need more shit here
		if (interp->err)
			return STATUS_ERROR;
		// TODO: check if there's a getter for the attribute for better error messages
		errorobject_setwithfmt(interp, "%s objects have no attribute named %D or it's read-only", data->name, stringobj);
		return STATUS_ERROR;
	}

	struct Object *res = functionobject_call(interp, setter, obj, val, NULL);
	OBJECT_DECREF(interp, setter);
	if (!res)
		return STATUS_ERROR;
	OBJECT_DECREF(interp, res);
	return STATUS_OK;
}

int attribute_set(struct Interpreter *interp, struct Object *obj, char *attr, struct Object *val)
{
	struct Object *stringobj = stringobject_newfromcharptr(interp, attr);
	if (!stringobj)
		return STATUS_ERROR;

	int res = attribute_setwithstringobj(interp, obj, stringobj, val);
	OBJECT_DECREF(interp, stringobj);
	return res;
}
