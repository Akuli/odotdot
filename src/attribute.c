// TODO: test these, or just write a lot of code that uses them and test that
#include "attribute.h"
#include "common.h"
#include "method.h"
#include "unicode.h"
#include "objects/errors.h"
#include "objects/mapping.h"
#include "objects/string.h"

static struct Object *get_the_attribute(struct Interpreter *interp, struct Object *obj, struct Object *attr)
{
	struct Object *res = mappingobject_get(interp, obj->attrs, attr);
	if (!res && !(interp->err)) {
		// key not found
		errorobject_setwithfmt(interp, "%D has no attribute named '%S'", obj, attr);
		return NULL;
	}
	return res;   // may be NULL
}

// TODO: this is too copy/pasta
struct Object *attribute_getwithustr(struct Interpreter *interp, struct Object *obj, struct UnicodeString attr)
{
	if (!obj->attrs) {
		errorobject_setwithfmt(interp, "%D has no attributes", obj);
		return NULL;
	}

	struct Object *s = stringobject_newfromustr(interp, attr);
	if (!s)
		return NULL;

	struct Object *res = get_the_attribute(interp, obj, s);
	OBJECT_DECREF(interp, s);
	return res;
}

// TODO: this is too copy/pasta
struct Object *attribute_get(struct Interpreter *interp, struct Object *obj, char *attr)
{
	if (!obj->attrs) {
		errorobject_setwithfmt(interp, "%D has no attributes", obj);
		return NULL;
	}

	struct Object *s = stringobject_newfromcharptr(interp, attr);
	if (!s)
		return NULL;

	struct Object *res = get_the_attribute(interp, obj, s);
	OBJECT_DECREF(interp, s);
	return res;
}


int attribute_set(struct Interpreter *interp, struct Object *obj, char *attr, struct Object *val)
{
	if (!obj->attrs) {
		errorobject_setwithfmt(interp, "%D has no attributes", obj);
		return STATUS_ERROR;
	}

	struct Object *s = stringobject_newfromcharptr(interp, attr);
	if (!s)
		return STATUS_ERROR;

	int setret = mappingobject_set(interp, obj->attrs, s, val);
	OBJECT_DECREF(interp, s);
	return setret;
}

// TODO: this is too copy/pasta
int attribute_setwithustr(struct Interpreter *interp, struct Object *obj, struct UnicodeString attr, struct Object *val)
{
	if (!obj->attrs) {
		errorobject_setwithfmt(interp, "%D has no attributes", obj);
		return STATUS_ERROR;
	}

	struct Object *s = stringobject_newfromustr(interp, attr);
	if (!s)
		return STATUS_ERROR;

	int setret = mappingobject_set(interp, obj->attrs, s, val);
	OBJECT_DECREF(interp, s);
	return setret;
}
