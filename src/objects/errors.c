#include "errors.h"
#include <stdarg.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include "classobject.h"
#include "string.h"
#include "../common.h"
#include "../interpreter.h"
#include "../objectsystem.h"
#include "../unicode.h"

static void error_foreachref(struct Object *obj, void *data, classobject_foreachrefcb cb)
{
	cb((struct Object *) obj->data, data);
}

struct Object *errorobject_createclass(struct Interpreter *interp)
{
	// Error objects can have any attributes
	// TODO: use a message attribute instead of ->data?
	return classobject_new_noerrptr(interp, "Error", interp->builtins.objectclass, 1, error_foreachref);
}

// TODO: stop copy/pasting this from string.c and actually fix things
static void string_destructor(struct Object *str)
{
        struct UnicodeString *data = str->data;
        free(data->val);
        free(data);
}

struct Object *errorobject_createnomemerr(struct Interpreter *interp)
{
	// message string is created here because string constructors use interp->nomemerr and errptr
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

	struct Object *str = object_new(interp, interp->builtins.stringclass, ustr, string_destructor);
	if (!str) {
		free(ustr->val);
		free(ustr);
		return NULL;
	}

	struct Object *err = object_new(interp, interp->builtins.errorclass, str, NULL);
	if (!err) {
		OBJECT_DECREF(interp, str);   // takes care of ustr and ustr->val
		return NULL;
	}
	return err;
}


void errorobject_setnomem(struct Interpreter *interp, struct Object **errptr)
{
	OBJECT_INCREF(interp, interp->builtins.nomemerr);
	*errptr = interp->builtins.nomemerr;
}

int errorobject_setwithfmt(struct Interpreter *interp, struct Object **errptr, char *fmt, ...)
{
	va_list ap;
	va_start(ap, fmt);
	struct Object *msg = stringobject_newfromvfmt(interp, errptr, fmt, ap);
	va_end(ap);
	if (!msg)
		return STATUS_ERROR;

	struct Object *err = classobject_newinstance(interp, errptr, interp->builtins.errorclass, msg, NULL);
	// don't decref msg, instead let err hold a reference to it
	if (!err) {
		OBJECT_DECREF(interp, msg);
		return STATUS_ERROR;
	}
	*errptr = err;
	return STATUS_OK;
}

int errorobject_typecheck(struct Interpreter *interp, struct Object **errptr, struct Object *klass, struct Object *obj)
{
	if (!classobject_instanceof(obj, klass)) {
		char *name = ((struct ClassObjectData*) klass->data)->name;
		errorobject_setwithfmt(interp, errptr, "should be an instance of %s, not %D", name, obj);
		return STATUS_ERROR;
	}
	return STATUS_OK;
}
