#include "errors.h"
#include <assert.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include "../check.h"
#include "../common.h"
#include "../interpreter.h"
#include "../method.h"
#include "../objectsystem.h"
#include "../unicode.h"
#include "array.h"
#include "classobject.h"
#include "mapping.h"
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

static struct Object *setup(struct Interpreter *interp, struct Object *argarr)
{
	if (check_args(interp, argarr, interp->builtins.Error, interp->builtins.String) == STATUS_ERROR)
		return NULL;

	struct Object *err = ARRAYOBJECT_GET(argarr, 0);
	if (err->data) {
		errorobject_setwithfmt(interp, "setup was called twice");
		return NULL;
	}

	err->data = ARRAYOBJECT_GET(argarr, 1);
	OBJECT_INCREF(interp, ARRAYOBJECT_GET(argarr, 1));
	return interpreter_getbuiltin(interp, "null");
}

int errorobject_addmethods(struct Interpreter *interp)
{
	// TODO: to_debug_string
	if (method_add(interp, interp->builtins.Error, "setup", setup) == STATUS_ERROR) return STATUS_ERROR;
	return STATUS_OK;
}

// TODO: stop copy/pasting this from string.c and actually fix things
static void string_destructor(struct Object *str)
{
        struct UnicodeString *data = str->data;
        free(data->val);
        free(data);
}

struct Object *errorobject_createnomemerr_noerr(struct Interpreter *interp)
{
	// message string is created here because string constructors use interp->nomemerr and interp->err
	// string objects are simple, the data is just a UnicodeString pointer
	struct UnicodeString *ustr = malloc(sizeof(struct UnicodeString));
	if (!ustr)
		return NULL;

	char msg[] = "not enough memory";
	ustr->len = strlen(msg);
	ustr->val = malloc(sizeof(unicode_char) * ustr->len);
	if (!(ustr->val)) {
		free(ustr);
		return NULL;
	}

	// can't use memcpy because different types
	for (size_t i=0; i < ustr->len; i++)
		ustr->val[i] = msg[i];

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


int errorobject_setwithfmt(struct Interpreter *interp, char *fmt, ...)
{
	va_list ap;
	va_start(ap, fmt);
	struct Object *msg = stringobject_newfromvfmt(interp, fmt, ap);
	va_end(ap);
	if (!msg)
		return STATUS_ERROR;

	struct Object *err = classobject_newinstance(interp, interp->builtins.Error, msg, NULL);
	// don't decref msg, instead let err hold a reference to it
	if (!err) {
		OBJECT_DECREF(interp, msg);
		return STATUS_ERROR;
	}
	interp->err = err;
	return STATUS_OK;
}
