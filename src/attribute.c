// TODO: test these, or just write a lot of code that uses them and test that
#include "attribute.h"
#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include "common.h"
#include "method.h"
#include "unicode.h"
#include "objects/classobject.h"
#include "objects/errors.h"
#include "objects/function.h"
#include "objects/mapping.h"
#include "objects/string.h"

static bool init_data(struct Interpreter *interp, struct Object *obj)
{
	if (!obj->attrdata) {
		obj->attrdata = mappingobject_newempty(interp);
		if (!obj->attrdata)
			return false;
	}
	return true;
}

int attribute_settoattrdata(struct Interpreter *interp, struct Object *obj, char *attr, struct Object *val)
{
	if (!init_data(interp, obj))
		return STATUS_ERROR;

	struct Object *stringobj = stringobject_newfromcharptr(interp, attr);
	if (!stringobj)
		return STATUS_ERROR;

	int res = mappingobject_set(interp, obj->attrdata, stringobj, val);
	OBJECT_DECREF(interp, stringobj);
	return res;
}

struct Object *attribute_getfromattrdata(struct Interpreter *interp, struct Object *obj, char *attr)
{
	if (!init_data(interp, obj))
		return NULL;

	struct Object *stringobj = stringobject_newfromcharptr(interp, attr);
	if (!stringobj)
		return NULL;

	// this sets a "key not found" error if the attribute is not found
	// mappingobject_get() sets no errors in that case
	struct Object *res = method_call(interp, obj->attrdata, "get", stringobj, NULL);
	OBJECT_DECREF(interp, stringobj);
	return res;
}


// returns true on success and false on failure
static bool init_class_mappings(struct Interpreter *interp, struct ClassObjectData *data)
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

int attribute_addwithfuncobjs(struct Interpreter *interp, struct Object *klass, char *name, struct Object *getter, struct Object *setter)
{
	assert(interp->builtins.mappingclass && interp->builtins.functionclass);   // must not be called tooo early by builtins_setup()
	assert(setter || getter);

	struct ClassObjectData *data = klass->data;
	if (!init_class_mappings(interp, data))
		return STATUS_ERROR;

	struct Object *string = stringobject_newfromcharptr(interp, name);
	if (!string)
		return STATUS_ERROR;

	// this is kinda copy/pasta, but... not 2 bad imo
	if (getter) {
		if (mappingobject_set(interp, data->getters, string, getter) == STATUS_ERROR) {
			OBJECT_DECREF(interp, string);
			return STATUS_ERROR;
		}
	}
	if (setter) {
		if (mappingobject_set(interp, data->setters, string, setter) == STATUS_ERROR) {
			OBJECT_DECREF(interp, string);
			return STATUS_ERROR;
		}
	}

	OBJECT_DECREF(interp, string);
	return STATUS_OK;
}

int attribute_add(struct Interpreter *interp, struct Object *klass, char *name, functionobject_cfunc getter, functionobject_cfunc setter)
{
	struct Object *getterobj = NULL, *setterobj = NULL;

	char prefixedname[sizeof("getter of ")-1 + strlen(name) + 1];
	memcpy(prefixedname, "getter of ", sizeof("getter of ")-1);
	strcpy(prefixedname + sizeof("getter of ")-1, name);

	if (getter) {
		if (!(getterobj = functionobject_new(interp, getter, prefixedname)))
			return STATUS_ERROR;
	}
	if (setter) {
		prefixedname[0] = 's';    // "setter of blabla" instead of "getter of blabla"
		if (!(setterobj = functionobject_new(interp, setter, prefixedname)))
			return STATUS_ERROR;
	}

	int res = attribute_addwithfuncobjs(interp, klass, name, getterobj, setterobj);
	if (getterobj) OBJECT_DECREF(interp, getterobj);
	if (setterobj) OBJECT_DECREF(interp, setterobj);
	return res;
}


struct Object *attribute_getwithstringobj(struct Interpreter *interp, struct Object *obj, struct Object *stringobj)
{
	struct Object *klass = obj->klass;
	struct ClassObjectData *klassdata;

	do {
		klassdata = klass->data;
		if (!init_class_mappings(interp, klassdata))
			return NULL;

		struct Object *getter;
		int res = mappingobject_get(interp, klassdata->getters, stringobj, &getter);
		if (res == -1)
			return NULL;
		if (res == 1) {
			struct Object *ret = functionobject_call(interp, getter, obj, NULL);
			OBJECT_DECREF(interp, getter);
			return ret;
		}
		assert(res == 0);   // not found
	} while ((klass = klassdata->baseclass));

	errorobject_setwithfmt(interp, "%s objects don't have an attribute named %D",
		((struct ClassObjectData *) obj->klass->data)->name, stringobj);
	return NULL;
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
	struct Object *klass = obj->klass;
	struct ClassObjectData *klassdata;

	do {
		klassdata = klass->data;
		if (!init_class_mappings(interp, klassdata))
			return STATUS_ERROR;

		struct Object *setter;
		int res = mappingobject_get(interp, klassdata->setters, stringobj, &setter);
		if (res == -1)
			return STATUS_ERROR;
		if (res == 1) {
			struct Object *res = functionobject_call(interp, setter, obj, val, NULL);
			OBJECT_DECREF(interp, setter);
			if (!res)
				return STATUS_ERROR;
			OBJECT_DECREF(interp, res);
			return STATUS_OK;
		}
		assert(res == 0);   // not found
	} while ((klass = klassdata->baseclass));

	// TODO: check if there's a getter for the attribute for better error messages
	errorobject_setwithfmt(interp, "%s objects don't have an attribute named %D or it's read-only",
		((struct ClassObjectData *) obj->klass->data)->name, stringobj);
	return STATUS_ERROR;
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
