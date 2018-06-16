#include "interpreter.h"
#include <assert.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include "allobjects.h"
#include "attribute.h"
#include "method.h"
#include "objectsystem.h"
#include "unicode.h"
#include "objects/errors.h"
#include "objects/mapping.h"
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
	interp->stackptr = interp->stack;   // make it point to the 1st element

	interp->err =
	interp->builtinscope =
	interp->builtins.Array =
	interp->builtins.AstNode =
	interp->builtins.Block =
	interp->builtins.Class =
	interp->builtins.Error =
	interp->builtins.Function =
	interp->builtins.Integer =
	interp->builtins.Mapping =
	interp->builtins.MarkerError =
	interp->builtins.Object =
	interp->builtins.StackFrame =
	interp->builtins.String =
	interp->builtins.null =
	interp->builtins.nomemerr =
	NULL;

	return interp;
}

void interpreter_free(struct Interpreter *interp)
{
	allobjects_free(interp->allobjects);
	free(interp);
}

bool interpreter_addbuiltin(struct Interpreter *interp, char *name, struct Object *val)
{
	struct Object *keystr = stringobject_newfromcharptr(interp, name);
	if (!keystr)
		return false;

	struct Object *localvars = attribute_get(interp, interp->builtinscope, "local_vars");
	if (!localvars) {
		OBJECT_DECREF(interp, keystr);
		return false;
	}

	bool ok = mappingobject_set(interp, localvars, keystr, val);
	OBJECT_DECREF(interp, keystr);
	OBJECT_DECREF(interp, localvars);
	return ok;
}

struct Object *interpreter_getbuiltin(struct Interpreter *interp, char *name)
{
	struct Object *keystr = stringobject_newfromcharptr(interp, name);
	if (!keystr)
		return NULL;

	struct Object *localvars = attribute_get(interp, interp->builtinscope, "local_vars");
	if (!localvars) {
		OBJECT_DECREF(interp, keystr);
		return NULL;
	}

	struct Object *ret = method_call(interp, localvars, "get", keystr, NULL);
	OBJECT_DECREF(interp, keystr);
	OBJECT_DECREF(interp, localvars);
	return ret;
}
