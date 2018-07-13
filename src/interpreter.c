#include "interpreter.h"
#include <assert.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include "allobjects.h"
#include "import.h"
#include "objectsystem.h"
#include "objects/errors.h"
#include "objects/mapping.h"
#include "objects/scope.h"
#include "objects/string.h"

struct Interpreter *interpreter_new(char *argv0)
{
	struct Interpreter *interp = malloc(sizeof(struct Interpreter));
	if (!interp)
		goto nomem;

	// initialize all members to NULLs, including nested structs
	*interp = (struct Interpreter){ 0 };

	if (!allobjects_init(&(interp->allobjects))) {
		free(interp);
		goto nomem;
	}

	interp->argv0 = argv0;
	interp->stackptr = interp->stack;   // make it point to the 1st element

	interp->stdpath = import_findstd(argv0);
	if (!interp->stdpath) {
		allobjects_free(interp->allobjects);
		free(interp);
		return NULL;
	}

	return interp;

nomem:
	fprintf(stderr, "%s: not enough memory\n", argv0);
	return NULL;
}

void interpreter_free(struct Interpreter *interp)
{
	allobjects_free(interp->allobjects);
	free(interp->stdpath);
	free(interp);
}

bool interpreter_addbuiltin(struct Interpreter *interp, char *name, struct Object *val)
{
	struct Object *keystr = stringobject_newfromcharptr(interp, name);
	if (!keystr)
		return false;

	bool ok = mappingobject_set(interp, SCOPEOBJECT_LOCALVARS(interp->builtinscope), keystr, val);
	OBJECT_DECREF(interp, keystr);
	return ok;
}

struct Object *interpreter_getbuiltin(struct Interpreter *interp, char *name)
{
	assert(interp->builtinscope);

	struct Object *keystr = stringobject_newfromcharptr(interp, name);
	if (!keystr)
		return NULL;

	struct Object *ret;
	int status = mappingobject_get(interp, SCOPEOBJECT_LOCALVARS(interp->builtinscope), keystr, &ret);
	OBJECT_DECREF(interp, keystr);
	if (status == 0)   // this recurses if someone deletes VariableError, but that's a bad idea anyway :D
		errorobject_throwfmt(interp, "VariableError", "cannot find a built-in variable named \"%s\"", name);
	if (status != 1)
		return NULL;
	return ret;
}
