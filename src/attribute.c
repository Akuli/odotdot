// these are not tested because there's lots of tested code that uses these
#include "attribute.h"
#include <assert.h>
#include <stdbool.h>
#include <string.h>
#include "check.h"
#include "interpreter.h"
#include "objectsystem.h"
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

bool attribute_settoattrdata(struct Interpreter *interp, struct Object *obj, char *attr, struct Object *val)
{
	if (!init_data(interp, obj))
		return false;

	struct Object *stringobj = stringobject_newfromcharptr(interp, attr);
	if (!stringobj)
		return false;

	bool res = mappingobject_set(interp, obj->attrdata, stringobj, val);
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

	struct Object *res;
	int status = mappingobject_get(interp, obj->attrdata, stringobj, &res);
	OBJECT_DECREF(interp, stringobj);
	if (status == 1)
		return res;
	if (status == 0) {
		// TODO: how is it possible to actually get this error?
		errorobject_throwfmt(interp, "AttribError", "%D has no \"%s\" attribute", obj, attr);
		return NULL;
	}
	assert(status == -1);
	return NULL;
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

bool attribute_addwithfuncobjs(struct Interpreter *interp, struct Object *klass, char *name, struct Object *getter, struct Object *setter)
{
	assert(interp->builtins.Mapping && interp->builtins.Function);   // must not be called tooo early by builtins_setup()
	assert(setter || getter);

	struct ClassObjectData *data = klass->objdata.data;
	if (!init_class_mappings(interp, data))
		return false;

	struct Object *string = stringobject_newfromcharptr(interp, name);
	if (!string)
		return false;

	// this is kinda copy/pasta, but... not 2 bad imo
	if (getter) {
		if (!mappingobject_set(interp, data->getters, string, getter)) {
			OBJECT_DECREF(interp, string);
			return false;
		}
	}
	if (setter) {
		if (!mappingobject_set(interp, data->setters, string, setter)) {
			OBJECT_DECREF(interp, string);
			return false;
		}
	}

	OBJECT_DECREF(interp, string);
	return true;
}

bool attribute_add(struct Interpreter *interp, struct Object *klass, char *name, functionobject_cfunc_yesret getter, functionobject_cfunc_noret setter)
{
	struct Object *getterobj = NULL, *setterobj = NULL;

	char prefixedname[sizeof("getter of ")-1 + strlen(name) + 1];
	memcpy(prefixedname, "getter of ", sizeof("getter of ")-1);
	strcpy(prefixedname + sizeof("getter of ")-1, name);

	struct ObjectData nulldata = { .data = NULL, .foreachref = NULL, .destructor = NULL };
	if (getter) {
		if (!(getterobj = functionobject_new(interp, nulldata, functionobject_mkcfunc_yesret(getter), prefixedname)))
			return false;
	}
	if (setter) {
		if (!(setterobj = functionobject_new(interp, nulldata, functionobject_mkcfunc_noret(setter), prefixedname)))
			return false;
	}

	bool res = attribute_addwithfuncobjs(interp, klass, name, getterobj, setterobj);
	if (getterobj) OBJECT_DECREF(interp, getterobj);
	if (setterobj) OBJECT_DECREF(interp, setterobj);
	return res;
}


struct Object *attribute_getwithstringobj(struct Interpreter *interp, struct Object *obj, struct Object *stringobj)
{
	struct Object *klass = obj->klass;
	struct ClassObjectData *klassdata;

	do {
		klassdata = klass->objdata.data;
		if (!init_class_mappings(interp, klassdata))
			return NULL;

		struct Object *getter;
		int res = mappingobject_get(interp, klassdata->getters, stringobj, &getter);
		if (res == -1)
			return NULL;
		if (res == 1) {
			if (!check_type(interp, interp->builtins.Function, getter)) {
				OBJECT_DECREF(interp, getter);
				return NULL;
			}
			struct Object *ret = functionobject_call_yesret(interp, getter, obj, NULL);
			OBJECT_DECREF(interp, getter);
			return ret;
		}
		assert(res == 0);   // not found
	} while ((klass = klassdata->baseclass));

	if (classobject_isinstanceof(obj, interp->builtins.ArbitraryAttribs)) {
		if (!init_data(interp, obj))
			return NULL;

		struct Object *val;
		int res = mappingobject_get(interp, obj->attrdata, stringobj, &val);
		if (res == 1)
			return val;
		if (res == 0)
			errorobject_throwfmt(interp, "AttribError", "%D has no %D attribute", obj, stringobj);
		return NULL;
	}

	errorobject_throwfmt(interp, "AttribError", "%U objects don't have an attribute named %D",
		((struct ClassObjectData *) obj->klass->objdata.data)->name, stringobj);
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


bool attribute_setwithstringobj(struct Interpreter *interp, struct Object *obj, struct Object *stringobj, struct Object *val)
{
	struct Object *klass = obj->klass;
	struct ClassObjectData *klassdata;

	do {
		klassdata = klass->objdata.data;
		if (!init_class_mappings(interp, klassdata))
			return false;

		struct Object *setter;
		int res = mappingobject_get(interp, klassdata->setters, stringobj, &setter);
		if (res == -1)
			return false;
		if (res == 1) {
			if (!check_type(interp, interp->builtins.Function, setter)) {
				OBJECT_DECREF(interp, setter);
				return false;
			}
			bool ok = functionobject_call_noret(interp, setter, obj, val, NULL);
			OBJECT_DECREF(interp, setter);
			return ok;
		}
		assert(res == 0);   // not found
	} while ((klass = klassdata->baseclass));

	if (classobject_isinstanceof(obj, interp->builtins.ArbitraryAttribs)) {
		if (!init_data(interp, obj))
			return false;
		return mappingobject_set(interp, obj->attrdata, stringobj, val);
	}

	// TODO: check if there's a getter for the attribute for better error messages
	errorobject_throwfmt(interp, "AttribError", "%U objects don't have an attribute named %D or it's read-only",
		((struct ClassObjectData *) obj->klass->objdata.data)->name, stringobj);
	return false;
}

bool attribute_set(struct Interpreter *interp, struct Object *obj, char *attr, struct Object *val)
{
	struct Object *stringobj = stringobject_newfromcharptr(interp, attr);
	if (!stringobj)
		return false;

	bool res = attribute_setwithstringobj(interp, obj, stringobj, val);
	OBJECT_DECREF(interp, stringobj);
	return res;
}
