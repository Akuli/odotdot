#include "interpreter.h"
#include <assert.h>
#include <string.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include "common.h"
#include "context.h"
#include "hashtable.h"
#include "objectsystem.h"
#include "unicode.h"

struct Interpreter *interpreter_new(char *argv0)
{
	struct Interpreter *interp = malloc(sizeof(struct Interpreter));
	if (!interp)
		return NULL;

	interp->argv0 = argv0;
	interp->nomemerr = NULL;
	interp->builtinctx = context_newglobal(interp);
	if (!(interp->builtinctx)) {
		free(interp);
		return NULL;
	}

	return interp;
}

void interpreter_free(struct Interpreter *interp)
{
	// TODO: how about all sub contexts? assume there are none??
	context_free(interp->builtinctx);
	free(interp);
}

int interpreter_addbuiltin(struct Interpreter *interp, struct Object **errptr, char *name, struct Object *val)
{
	struct UnicodeString uname;
	char stupiderrormsg[100] = {0};
	int status = utf8_decode(name, strlen(name), &uname, stupiderrormsg);
	if (status == STATUS_NOMEM) {
		*errptr = interp->nomemerr;
		return STATUS_ERROR;
	}
	assert(status == STATUS_OK && stupiderrormsg[0] == 0);  // must be valid utf8

	return context_setlocalvar(interp->builtinctx, errptr, uname, val);
}

struct Object *interpreter_getbuiltin(struct Interpreter *interp, struct Object **errptr, char *name)
{
	struct UnicodeString uname;
	char stupiderrormsg[100] = {0};
	int status = utf8_decode(name, strlen(name), &uname, stupiderrormsg);
	if (status == STATUS_NOMEM) {
		*errptr = interp->nomemerr;
		return NULL;
	}
	assert(status == STATUS_OK && stupiderrormsg[0] == 0);  // must be valid utf8

	return context_getvar(interp->builtinctx, errptr, uname);
}
