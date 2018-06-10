#include "errors.h"
#include <assert.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include "../check.h"
#include "../interpreter.h"
#include "../method.h"
#include "../objectsystem.h"
#include "../unicode.h"
#include "array.h"
#include "classobject.h"
#include "mapping.h"
#include "null.h"
#include "string.h"

static void error_foreachref(struct Object *obj, void *data, classobject_foreachrefcb cb)
{
	if (obj->data)
		cb((struct Object *) obj->data, data);
}

struct Object *errorobject_createclass_noerr(struct Interpreter *interp)
{
	// Error objects can have any attributes
	// TODO: use a message attribute instead of ->data?
	return classobject_new_noerr(interp, "Error", interp->builtins.Object, error_foreachref);
}

static struct Object *setup(struct Interpreter *interp, struct Object *args, struct Object *opts)
{
	if (!check_args(interp, args, interp->builtins.Error, interp->builtins.String)) return NULL;
	if (!check_no_opts(interp, opts)) return NULL;

	struct Object *err = ARRAYOBJECT_GET(args, 0);
	if (err->data) {
		errorobject_setwithfmt(interp, "setup was called twice");
		return NULL;
	}

	err->data = ARRAYOBJECT_GET(args, 1);
	OBJECT_INCREF(interp, ARRAYOBJECT_GET(args, 1));
	return nullobject_get(interp);
}

bool errorobject_addmethods(struct Interpreter *interp)
{
	// TODO: to_debug_string
	if (!method_add(interp, interp->builtins.Error, "setup", setup)) return false;
	return true;
}

// TODO: stop copy/pasting this from string.c and actually fix things
static void string_destructor(struct Object *str)
{
        struct UnicodeString *data = str->data;
        free(data->val);
        free(data);
}

#define MESSAGE "not enough memory"
struct Object *errorobject_createnomemerr_noerr(struct Interpreter *interp)
{
	// message string is created here because string constructors use interp->nomemerr and interp->err
	// string objects are simple, the data is just a UnicodeString pointer
	struct UnicodeString *ustr = malloc(sizeof(struct UnicodeString));
	if (!ustr)
		return NULL;

	ustr->len = sizeof(MESSAGE) - 1;
	ustr->val = malloc(sizeof(unicode_char) * ustr->len);
	if (!(ustr->val)) {
		free(ustr);
		return NULL;
	}

	// can't use memcpy because different types
	for (size_t i=0; i < ustr->len; i++)
		ustr->val[i] = MESSAGE[i];

	struct Object *str = object_new_noerr(interp, interp->builtins.String, ustr, string_destructor);
	if (!str) {
		free(ustr->val);
		free(ustr);
		return NULL;
	}

	struct Object *err = object_new_noerr(interp, interp->builtins.Error, str, NULL);
	if (!err) {
		OBJECT_DECREF(interp, str);   // takes care of ustr and ustr->val
		return NULL;
	}
	return err;
}
#undef MESSAGE


bool errorobject_setwithfmt(struct Interpreter *interp, char *fmt, ...)
{
	va_list ap;
	va_start(ap, fmt);
	struct Object *msg = stringobject_newfromvfmt(interp, fmt, ap);
	va_end(ap);
	if (!msg)
		return false;

	struct Object *err = classobject_newinstance(interp, interp->builtins.Error, msg, NULL);
	// don't decref msg, instead let err hold a reference to it
	if (!err) {
		OBJECT_DECREF(interp, msg);
		return false;
	}
	interp->err = err;
	return true;
}
