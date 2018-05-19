#include "builtins.h"
#include <assert.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include "ast.h"
#include "common.h"
#include "context.h"
#include "interpreter.h"
#include "unicode.h"
#include "objectsystem.h"
#include "objects/array.h"
#include "objects/classobject.h"
#include "objects/errors.h"
#include "objects/function.h"
#include "objects/integer.h"
#include "objects/mapping.h"
#include "objects/object.h"
#include "objects/string.h"


static struct Object *print_builtin(struct Interpreter *interp, struct Object **errptr, struct Object **args, size_t nargs)
{
	if (functionobject_checktypes(interp, errptr, args, nargs, interp->builtins.stringclass, NULL) == STATUS_ERROR)
		return NULL;

	char *utf8;
	size_t utf8len;
	char errormsg[100];
	int status = utf8_encode(*((struct UnicodeString *) args[0]->data), &utf8, &utf8len, errormsg);
	if (status == STATUS_NOMEM) {
		errorobject_setnomem(interp, errptr);
		return NULL;
	}
	assert(status == STATUS_OK);  // TODO: how about invalid unicode strings? make sure they don't exist when creating strings?

	// TODO: avoid writing 1 byte at a time... seems to be hard with c \0 strings
	for (size_t i=0; i < utf8len; i++)
		putchar(utf8[i]);
	free(utf8);
	putchar('\n');

	// this must return a new reference on success
	return stringobject_newfromcharptr(interp, errptr, "asd");
}


int builtins_setup(struct Interpreter *interp)
{
	if (!(interp->builtinctx = context_newglobal(interp))) return STATUS_NOMEM;

	if (!(interp->builtins.objectclass = objectobject_createclass(interp))) return STATUS_NOMEM;
	if (!(interp->builtins.classclass = classobject_create_classclass(interp, interp->builtins.objectclass))) return STATUS_NOMEM;

	interp->builtins.objectclass->klass = interp->builtins.classclass;
	OBJECT_INCREF(interp, interp->builtins.classclass);
	interp->builtins.classclass->klass = interp->builtins.classclass;
	OBJECT_INCREF(interp, interp->builtins.classclass);

	if (!(interp->builtins.stringclass = stringobject_createclass(interp))) return STATUS_NOMEM;
	if (!(interp->builtins.errorclass = errorobject_createclass(interp))) return STATUS_NOMEM;
	if (!(interp->builtins.nomemerr = errorobject_createnomemerr(interp))) return STATUS_NOMEM;

	struct Object *err;
	if (!(interp->builtins.functionclass = functionobject_createclass(interp, &err))) goto error;
	if (objectobject_addmethods(interp, &err) == STATUS_ERROR) goto error;

	if (!(interp->builtins.mappingclass = mappingobject_createclass(interp, &err))) goto error;
	if (stringobject_addmethods(interp, &err) == STATUS_ERROR) goto error;

	if (!(interp->builtins.print = functionobject_new(interp, &err, print_builtin))) goto error;
	if (!(interp->builtins.arrayclass = arrayobject_createclass(interp, &err))) goto error;
	if (!(interp->builtins.integerclass = integerobject_createclass(interp, &err))) goto error;
	if (!(interp->builtins.astnodeclass = astnode_createclass(interp, &err))) goto error;

	if (interpreter_addbuiltin(interp, &err, "Array", interp->builtins.arrayclass)) goto error;
	if (interpreter_addbuiltin(interp, &err, "Error", interp->builtins.errorclass)) goto error;
	if (interpreter_addbuiltin(interp, &err, "Integer", interp->builtins.integerclass)) goto error;
	if (interpreter_addbuiltin(interp, &err, "Mapping", interp->builtins.mappingclass)) goto error;
	if (interpreter_addbuiltin(interp, &err, "Object", interp->builtins.objectclass)) goto error;
	if (interpreter_addbuiltin(interp, &err, "String", interp->builtins.stringclass)) goto error;
	if (interpreter_addbuiltin(interp, &err, "print", interp->builtins.print)) goto error;

	return STATUS_OK;

error:
	fprintf(stderr, "an error occurred :(\n");    // TODO: better error message printing!
	OBJECT_DECREF(interp, err);
	return STATUS_ERROR;
}


void builtins_teardown(struct Interpreter *interp)
{
#define TEARDOWN(x) if (interp->builtins.x) { OBJECT_DECREF(interp, interp->builtins.x); interp->builtins.x = NULL; }
	TEARDOWN(astnodeclass);
	TEARDOWN(functionclass);
	TEARDOWN(nomemerr);
	TEARDOWN(classclass);
	TEARDOWN(arrayclass);
	TEARDOWN(errorclass);
	TEARDOWN(integerclass);
	TEARDOWN(mappingclass);
	TEARDOWN(objectclass);
	TEARDOWN(stringclass);
	TEARDOWN(print);
#undef TEARDOWN

	if (interp->builtinctx) {
		context_free(interp->builtinctx);
		interp->builtinctx = NULL;
	}
}
