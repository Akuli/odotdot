// TODO: test these, or just write a lot of code that uses them and test that
#include "attribute.h"
#include "common.h"
#include "method.h"
#include "unicode.h"
#include "objects/errors.h"
#include "objects/mapping.h"
#include "objects/string.h"

static struct Object *get_the_attribute(struct Interpreter *interp, struct Object **errptr, struct Object *obj, struct Object *attr)
{
	struct Object *res = mappingobject_get(interp, errptr, obj->attrs, attr);
	if (!res && !*errptr) {
		// key not found
		errorobject_setwithfmt(interp, errptr, "%D has no attribute named '%S'", obj, attr);
		return NULL;
	}
	return res;   // may be NULL
}

// TODO: this is too copy/pasta
struct Object *attribute_getwithustr(struct Interpreter *interp, struct Object **errptr, struct Object *obj, struct UnicodeString attr)
{
	if (!obj->attrs) {
		errorobject_setwithfmt(interp, errptr, "%D has no attributes", obj);
		return NULL;
	}

	struct Object *s = stringobject_newfromustr(interp, errptr, attr);
	if (!s)
		return NULL;

	struct Object *res = get_the_attribute(interp, errptr, obj, s);
	OBJECT_DECREF(interp, s);
	return res;
}

// TODO: this is too copy/pasta
struct Object *attribute_get(struct Interpreter *interp, struct Object **errptr, struct Object *obj, char *attr)
{
	if (!obj->attrs) {
		errorobject_setwithfmt(interp, errptr, "%D has no attributes", obj);
		return NULL;
	}

	struct Object *s = stringobject_newfromcharptr(interp, errptr, attr);
	if (!s)
		return NULL;

	struct Object *res = get_the_attribute(interp, errptr, obj, s);
	OBJECT_DECREF(interp, s);
	return res;
}


int attribute_set(struct Interpreter *interp, struct Object **errptr, struct Object *obj, char *attr, struct Object *val)
{
	if (!obj->attrs) {
		errorobject_setwithfmt(interp, errptr, "%D has no attributes", obj);
		return STATUS_ERROR;
	}

	struct Object *s = stringobject_newfromcharptr(interp, errptr, attr);
	if (!s)
		return STATUS_ERROR;

	int setret = mappingobject_set(interp, errptr, obj->attrs, s, val);
	OBJECT_DECREF(interp, s);
	return setret;
}
