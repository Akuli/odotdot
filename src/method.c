#include "method.h"
#include <assert.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include "common.h"
#include "unicode.h"

int method_add(struct Interpreter *interp, struct Object **errptr, struct Object *klass, char *name, functionobject_cfunc cfunc)
{
	assert(klass->klass == interp->classclass);    // TODO: better type check

	struct UnicodeString *uname = malloc(sizeof(struct UnicodeString));
	if (!uname) {
		*errptr = interp->nomemerr;
		return STATUS_ERROR;
	}

	char errormsg[100];
	int status = utf8_decode(name, strlen(name), uname, errormsg);
	if (status != STATUS_OK) {
		assert(status == STATUS_NOMEM);
		*errptr = interp->nomemerr;
		free(uname);
		return STATUS_ERROR;
	}

	struct Object *func = functionobject_new(interp, errptr, cfunc, NULL);
	if (!func) {
		free(uname->val);
		free(uname);
		return STATUS_ERROR;
	}

	if (hashtable_set(((struct ObjectClassInfo *) klass->data)->methods, uname, unicodestring_hash(*uname), func, NULL) == STATUS_NOMEM) {
		*errptr = interp->nomemerr;
		free(uname->val);
		free(uname);
		OBJECT_DECREF(interp, func);
		return STATUS_ERROR;
	}

	// don't decref func, now klass->data->methods holds the reference
	// don't free uname or uname->val because uname is now used as a key in info->methods
	return STATUS_OK;
}

struct Object *method_getwithustr(struct Interpreter *interp, struct Object **errptr, struct Object *obj, struct UnicodeString uname)
{
	// TODO: inheritance??
	struct Object *nopartial;
	int found = hashtable_get(((struct ObjectClassInfo *) obj->klass->data)->methods, &uname, unicodestring_hash(uname), (void **)(&nopartial), NULL);
	assert(found);    // TODO: set errptr instead :(((

	// now we have a function that takes self as the first argument, let's partial it
	// no need to decref nopartial because hashtable_get() doesn't incref
	// functionobject_newpartial() will take care of the rest
	return functionobject_newpartial(interp, errptr, nopartial, obj);
}

struct Object *method_get(struct Interpreter *interp, struct Object **errptr, struct Object *obj, char *name)
{
	struct UnicodeString *uname = malloc(sizeof(struct UnicodeString));
	if (!uname) {
		*errptr = interp->nomemerr;
		return NULL;
	}

	char errormsg[100];
	int status = utf8_decode(name, strlen(name), uname, errormsg);
	if (status != STATUS_OK) {
		assert(status == STATUS_NOMEM);
		*errptr = interp->nomemerr;
		free(uname);
		return NULL;
	}

	struct Object *res = method_getwithustr(interp, errptr, obj, *uname);
	free(uname->val);
	free(uname);
	return res;
}


#define NARGS_MAX 20   // same as in objects/function.c
struct Object *method_call(struct Context *ctx, struct Object **errptr, struct Object *obj, char *methname, ...)
{
	struct Object *method = method_get(ctx->interp, errptr, obj, methname);
	if (!method)
		return NULL;

	struct Object *args[NARGS_MAX];
	va_list ap;
	va_start(ap, methname);

	int nargs;
	for (nargs = 0; nargs < NARGS_MAX; nargs++) {
		struct Object *arg = va_arg(ap, struct Object*);
		if (!arg)
			break;
		args[nargs] = arg;
	}
	va_end(ap);

	struct Object *res = functionobject_vcall(ctx, errptr, method, args, nargs);
	OBJECT_DECREF(ctx->interp, method);
	return res;
}
#undef NARGS_MAX
