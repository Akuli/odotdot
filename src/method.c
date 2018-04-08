#include "method.h"
#include <assert.h>
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
	if (!found)
		return NULL;

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
