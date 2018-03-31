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
	interp->globalctx = context_newglobal(interp);
	if (!(interp->globalctx)) {
		free(interp);
		return NULL;
	}

	return interp;
}

void interpreter_free(struct Interpreter *interp)
{
	// TODO: how about all sub contexts? assume there are none??
	context_free(interp->globalctx);
	free(interp);
}
