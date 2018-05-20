#include "interpreter.h"
#include <assert.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include "allobjects.h"
#include "attribute.h"
#include "common.h"
#include "method.h"
#include "objectsystem.h"
#include "unicode.h"
#include "objects/errors.h"
#include "objects/string.h"

struct Interpreter *interpreter_new(char *argv0)
{
	struct Interpreter *interp = malloc(sizeof(struct Interpreter));
	if (!interp)
		return NULL;

	if (!allobjects_init(&(interp->allobjects))) {
		free(interp);
		return NULL;
	}

	interp->argv0 = argv0;

	interp->builtinscope =
	interp->builtins.arrayclass =
	interp->builtins.astnodeclass =
	interp->builtins.blockclass =
	interp->builtins.classclass =
	interp->builtins.errorclass =
	interp->builtins.functionclass =
	interp->builtins.integerclass =
	interp->builtins.mappingclass =
	interp->builtins.objectclass =
	interp->builtins.stringclass =

	interp->builtins.array_func =
	interp->builtins.nomemerr =
	interp->builtins.print =
	NULL;

	return interp;
}

void interpreter_free(struct Interpreter *interp)
{
	allobjects_free(interp->allobjects);
	free(interp);
}

int interpreter_addbuiltin(struct Interpreter *interp, struct Object **errptr, char *name, struct Object *val)
{
	struct Object *keystr = stringobject_newfromcharptr(interp, errptr, name);
	if (!keystr)
		return STATUS_ERROR;

	struct Object *localvars = attribute_get(interp, errptr, interp->builtinscope, "local_vars");
	if (!localvars) {
		OBJECT_DECREF(interp, keystr);
		return STATUS_ERROR;
	}

	struct Object *ret = method_call(interp, errptr, localvars, "set", keystr, val, NULL);
	OBJECT_DECREF(interp, keystr);
	OBJECT_DECREF(interp, localvars);
	if (ret) {
		OBJECT_DECREF(interp, ret);
		return STATUS_OK;
	}
	return STATUS_ERROR;
}

struct Object *interpreter_getbuiltin(struct Interpreter *interp, struct Object **errptr, char *name)
{
	struct Object *keystr = stringobject_newfromcharptr(interp, errptr, name);
	if (!keystr)
		return NULL;

	struct Object *localvars = attribute_get(interp, errptr, interp->builtinscope, "local_vars");
	if (!localvars) {
		OBJECT_DECREF(interp, keystr);
		return NULL;
	}

	struct Object *ret = method_call(interp, errptr, localvars, "get", keystr, NULL);
	OBJECT_DECREF(interp, keystr);
	OBJECT_DECREF(interp, localvars);
	return ret;
}
