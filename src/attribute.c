// TODO: test these, or just write a lot of code that uses them and test that
#include "attribute.h"
#include "common.h"
#include "method.h"
#include "unicode.h"
#include "objects/errors.h"
#include "objects/string.h"

struct Object *attribute_get(struct Interpreter *interp, struct Object **errptr, struct Object *obj, char *attr)
{
	if (!obj->attrs) {
		errorobject_setwithfmt(interp, errptr, "%D has no attributes", obj);
		return NULL;
	}

	struct Object *s = stringobject_newfromcharptr(interp, errptr, attr);
	if (!s)
		return NULL;

	struct Object *res = method_call(interp, errptr, obj->attrs, "get", s, NULL);
	OBJECT_DECREF(interp, s);
	return res;   // may be NULL
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

	struct Object *res = method_call(interp, errptr, obj->attrs, "set", s, val, NULL);
	OBJECT_DECREF(interp, s);
	if (!res)
		return STATUS_ERROR;

	OBJECT_DECREF(interp, res);
	return STATUS_OK;
}
