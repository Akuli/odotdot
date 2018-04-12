#include "errors.h"
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include "classobject.h"
#include "string.h"
#include "../common.h"
#include "../context.h"
#include "../interpreter.h"
#include "../objectsystem.h"
#include "../unicode.h"

static void error_foreachref(struct Object *obj, void *data, objectclassinfo_foreachrefcb cb)
{
	cb((struct Object *) obj->data, data);
}

struct ObjectClassInfo *errorobject_createclass(struct ObjectClassInfo *objectclass)
{
	return objectclassinfo_new("Error", objectclass, error_foreachref, NULL);
}

// message string is created here because string constructors want to use interp->nomemerr and errptr
struct Object *errorobject_createnomemerr(struct Interpreter *interp, struct ObjectClassInfo *errorclass, struct ObjectClassInfo *stringclass)
{
	struct UnicodeString *ustr = malloc(sizeof(struct UnicodeString));
	if (!ustr)
		return NULL;

	char msg[] = "not enough memory";
	ustr->len = strlen(msg);
	ustr->val = malloc(sizeof(uint32_t) * ustr->len);
	if (!(ustr->val)) {
		free(ustr);
		return NULL;
	}

	// can't use memcpy because different types
	for (size_t i=0; i < ustr->len; i++)
		ustr->val[i] = msg[i];

	struct Object *str = object_new(interp, NULL, ustr);
	if (!str) {
		free(ustr->val);
		free(ustr);
		return NULL;
	}

	struct Object *err = object_new(interp, NULL, str);
	if (!err) {
		OBJECT_DECREF(interp, str);   // takes care of ustr and ustr->val
		return NULL;
	}
	return err;
}


void errorobject_setnomem(struct Interpreter *interp, struct Object **errptr)
{
	OBJECT_INCREF(interp, interp->nomemerr);
	*errptr = interp->nomemerr;
}

int errorobject_setwithfmt(struct Context *ctx, struct Object **errptr, char *fmt, ...)
{
	struct Object *errorclass = interpreter_getbuiltin(ctx->interp, errptr, "Error");
	if (!errorclass)
		return STATUS_ERROR;

	va_list ap;
	va_start(ap, fmt);
	struct Object *msg = stringobject_newfromvfmt(ctx, errptr, fmt, ap);
	va_end(ap);
	if (!msg) {
		OBJECT_DECREF(ctx->interp, errorclass);
		return STATUS_ERROR;
	}

	struct Object *err = classobject_newinstance(ctx->interp, errptr, errorclass, msg);
	OBJECT_DECREF(ctx->interp, errorclass);
	// don't decref msg, instead let err hold a reference to it
	if (!err) {
		OBJECT_DECREF(ctx->interp, msg);
		return STATUS_ERROR;
	}
	*errptr = err;
	return STATUS_OK;
}

int errorobject_typecheck(struct Context *ctx, struct Object **errptr, struct Object *klass, struct Object *obj)
{
	// make sure that klass is a class, but avoid infinite recursion
	if (klass != ctx->interp->classclass) {
		if (errorobject_typecheck(ctx, errptr, ctx->interp->classclass, klass) == STATUS_ERROR)
			return STATUS_ERROR;
	}

	if (!classobject_istypeof(klass, obj)) {
		char *name = ((struct ObjectClassInfo*) klass->data)->name;
		uint32_t first = name[0];    // FIXME
		errorobject_setwithfmt(ctx, errptr, "should be %s %s, not %D", unicode_iswovel(first) ? "an" : "a", name, obj);
		return STATUS_ERROR;
	}
	return STATUS_OK;
}
