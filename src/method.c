#include "method.h"
#include <assert.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include "common.h"
#include "interpreter.h"
#include "hashtable.h"
#include "objects/classobject.h"
#include "objects/errors.h"
#include "objects/function.h"
#include "objects/integer.h"
#include "objectsystem.h"
#include "unicode.h"

int method_add(struct Interpreter *interp, struct Object **errptr, struct Object *klass, char *name, functionobject_cfunc cfunc)
{
	struct UnicodeString *uname = malloc(sizeof(struct UnicodeString));
	if (!uname) {
		errorobject_setnomem(interp, errptr);
		return STATUS_ERROR;
	}

	char errormsg[100];
	int status = utf8_decode(name, strlen(name), uname, errormsg);
	if (status != STATUS_OK) {
		assert(status == STATUS_NOMEM);
		free(uname);
		errorobject_setnomem(interp, errptr);
		return STATUS_ERROR;
	}

	struct Object *func = functionobject_new(interp, errptr, cfunc);
	if (!func) {
		free(uname->val);
		free(uname);
		return STATUS_ERROR;
	}

	if (hashtable_set(((struct ClassObjectData *) klass->data)->methods, uname, unicodestring_hash(*uname), func, NULL) == STATUS_NOMEM) {
		free(uname->val);
		free(uname);
		OBJECT_DECREF(interp, func);
		errorobject_setnomem(interp, errptr);
		return STATUS_ERROR;
	}

	// don't decref func, now klass->data->methods holds the reference
	// don't free uname or uname->val because uname is now used as a key in ->methods
	return STATUS_OK;
}


// does NOT return a new reference
static struct Object *get_the_method(struct Object *klass, struct UnicodeString *uname, unsigned int unamehash)
{
	struct Object *res;
	struct ClassObjectData *data;
	do {
		data = klass->data;
		if (hashtable_get(data->methods, uname, unamehash, (void **)(&res), NULL))
			return res;
	} while ((klass = data->baseclass));

	// nope
	return NULL;
}

struct Object *method_getwithustr(struct Interpreter *interp, struct Object **errptr, struct Object *obj, struct UnicodeString uname)
{
	struct Object *nopartial = get_the_method(obj->klass, &uname, unicodestring_hash(uname));
	if (!nopartial) {
		// TODO: test this
		char *classname = ((struct ClassObjectData*) obj->klass->data)->name;
		errorobject_setwithfmt(interp, errptr, "%s objects don't have a method named '%U'", classname, uname);
		return NULL;
	}

	// now we have a function that takes self as the first argument, let's partial it
	// no need to decref nopartial because get_the_method() doesn't incref
	return functionobject_newpartial(interp, errptr, nopartial, obj);
}

struct Object *method_get(struct Interpreter *interp, struct Object **errptr, struct Object *obj, char *name)
{
	struct UnicodeString uname;

	char errormsg[100];
	int status = utf8_decode(name, strlen(name), &uname, errormsg);
	if (status != STATUS_OK) {
		assert(status == STATUS_NOMEM);
		errorobject_setnomem(interp, errptr);
		return NULL;
	}

	struct Object *res = method_getwithustr(interp, errptr, obj, uname);
	free(uname.val);
	return res;
}


#define NARGS_MAX 20   // same as in objects/function.c
struct Object *method_call(struct Interpreter *interp, struct Object **errptr, struct Object *obj, char *methname, ...)
{
	struct Object *method = method_get(interp, errptr, obj, methname);
	if (!method)
		return NULL;

	struct Object *args[NARGS_MAX];
	va_list ap;
	va_start(ap, methname);

	int nargs;
	for (nargs = 0; nargs < NARGS_MAX; nargs++) {
		struct Object *arg = va_arg(ap, struct Object*);
		if (!arg)
			break;   // end of argument list, not an error
		args[nargs] = arg;
	}
	va_end(ap);

	struct Object *res = functionobject_vcall(interp, errptr, method, args, nargs);
	OBJECT_DECREF(interp, method);
	return res;
}
#undef NARGS_MAX

static struct Object *to_maybe_debug_string(struct Interpreter *interp, struct Object **errptr, struct Object *obj, char *methname)
{
	struct Object *res = method_call(interp, errptr, obj, methname, NULL);
	if (!res)
		return NULL;

	// this doesn't use errorobject_typecheck() because this uses a custom error message string
	if (!classobject_instanceof(res, interp->builtins.stringclass)) {
		// FIXME: is it possible to make this recurse infinitely by returning the object itself from to_{debug,}string?
		errorobject_setwithfmt(interp, errptr, "%s should return a String, but it returned %D", methname, res);
		OBJECT_DECREF(interp, res);
		return NULL;
	}
	return res;
}

struct Object *method_call_tostring(struct Interpreter *interp, struct Object **errptr, struct Object *obj)
{
	return to_maybe_debug_string(interp, errptr, obj, "to_string");
}

struct Object *method_call_todebugstring(struct Interpreter *interp, struct Object **errptr, struct Object *obj)
{
	return to_maybe_debug_string(interp, errptr, obj, "to_debug_string");
}

