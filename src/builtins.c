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
#include "objects/object.h"
#include "objects/string.h"


static struct Object *print_builtin(struct Interpreter *interp, struct Object **errptr, struct Object **args, size_t nargs)
{
	if (functionobject_checktypes(interp, errptr, args, nargs, "String", NULL) == STATUS_ERROR)
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
	interp->builtinctx = context_newglobal(interp);
	if (!interp->builtinctx)
		return STATUS_NOMEM;

	struct Object *objectclass = objectobject_createclass(interp);
	if (!objectclass)
		return STATUS_NOMEM;

	interp->classclass = classobject_create_classclass(interp, objectclass);
	if (!interp->classclass) {
		OBJECT_DECREF(interp, objectclass);
		return STATUS_NOMEM;
	}
	objectclass->klass = interp->classclass;
	OBJECT_INCREF(interp, interp->classclass);
	interp->classclass->klass = interp->classclass;
	OBJECT_INCREF(interp, interp->classclass);

	struct Object *stringclass = stringobject_createclass(interp, objectclass);
	if (!stringclass) {
		OBJECT_DECREF(interp, objectclass);
		return STATUS_NOMEM;
	}

	struct Object *errorclass = errorobject_createclass(interp, objectclass);
	if (!errorclass) {
		OBJECT_DECREF(interp, stringclass);
		OBJECT_DECREF(interp, objectclass);
		return STATUS_NOMEM;
	}

	interp->nomemerr = errorobject_createnomemerr(interp, errorclass, stringclass);
	if (!interp->nomemerr) {
		OBJECT_DECREF(interp, errorclass);
		OBJECT_DECREF(interp, stringclass);
		OBJECT_DECREF(interp, objectclass);
		return STATUS_NOMEM;
	}

	struct Object *err = NULL;
	if (interpreter_addbuiltin(interp, &err, "Object", objectclass) == STATUS_ERROR ||
		interpreter_addbuiltin(interp, &err, "String", stringclass) == STATUS_ERROR ||
		interpreter_addbuiltin(interp, &err, "Error", errorclass) == STATUS_ERROR) {
		// one of them failed
		assert(err);
		fprintf(stderr, "an error occurred :(\n");    // TODO: better error message printing!
		OBJECT_DECREF(interp, err);

		OBJECT_DECREF(interp, errorclass);
		OBJECT_DECREF(interp, stringclass);
		OBJECT_DECREF(interp, objectclass);
		return STATUS_ERROR;
	}
	assert(!err);

	// now the interpreter's builtin mapping holds references to these
	OBJECT_DECREF(interp, errorclass);
	OBJECT_DECREF(interp, stringclass);
	OBJECT_DECREF(interp, objectclass);

	interp->functionclass = functionobject_createclass(interp, &err);
	if (!interp->functionclass) {
		fprintf(stderr, "an error occurred :(\n");    // TODO: better error message printing!
		OBJECT_DECREF(interp, err);
		return STATUS_ERROR;
	}

	if (objectobject_addmethods(interp, &err) == STATUS_ERROR) {
		fprintf(stderr, "an error occurred :(\n");    // TODO: better error message printing!
		OBJECT_DECREF(interp, err);
		return STATUS_ERROR;
	}

	// now the hard stuff is done \o/ let's do some easier things

	if (stringobject_addmethods(interp, &err) == STATUS_ERROR) {
		fprintf(stderr, "an error occurred :(\n");    // TODO: better error message printing!
		OBJECT_DECREF(interp, err);
		return STATUS_ERROR;
	}

	struct Object *print = functionobject_new(interp, &err, print_builtin);
	if (!print) {
		fprintf(stderr, "an error occurred :(\n");    // TODO: better error message printing!
		OBJECT_DECREF(interp, err);
		return STATUS_ERROR;
	}
	if (interpreter_addbuiltin(interp, &err, "print", print) == STATUS_ERROR) {
		fprintf(stderr, "an error occurred :(\n");    // TODO: better error message printing!
		OBJECT_DECREF(interp, err);
		OBJECT_DECREF(interp, print);
		return STATUS_ERROR;
	}
	OBJECT_DECREF(interp, print);

	struct Object *arrayclass = arrayobject_createclass(interp, &err);
	if (!arrayclass) {
		fprintf(stderr, "an error occurred :(\n");    // TODO: better error message printing!
		OBJECT_DECREF(interp, err);
		return STATUS_ERROR;
	}
	if (interpreter_addbuiltin(interp, &err, "Array", arrayclass) == STATUS_ERROR) {
		fprintf(stderr, "an error occurred :(\n");    // TODO: better error message printing!
		OBJECT_DECREF(interp, err);
		OBJECT_DECREF(interp, arrayclass);
		return STATUS_ERROR;
	}
	OBJECT_DECREF(interp, arrayclass);

	struct Object *integerclass = integerobject_createclass(interp, &err);
	if (!integerclass) {
		fprintf(stderr, "an error occurred :(\n");    // TODO: better error message printing!
		OBJECT_DECREF(interp, err);
		return STATUS_ERROR;
	}
	if (interpreter_addbuiltin(interp, &err, "Integer", integerclass) == STATUS_ERROR) {
		fprintf(stderr, "an error occurred :(\n");    // TODO: better error message printing!
		OBJECT_DECREF(interp, err);
		OBJECT_DECREF(interp, integerclass);
		return STATUS_ERROR;
	}
	OBJECT_DECREF(interp, integerclass);

	interp->astnodeclass = astnode_createclass(interp, &err);
	if (!interp->astnodeclass) {
		fprintf(stderr, "an error occurred :(\n");    // TODO: better error message printing!
		OBJECT_DECREF(interp, err);
		return STATUS_ERROR;
	}

	/*
	printf("*** classes added by builtins_setup() ***\n");
	struct Object *classes[] = { objectclass, interp->classclass, stringclass, errorclass, interp->functionclass, arrayclass, integerclass, interp->astnodeclass };
	for (unsigned int i=0; i < sizeof(classes) / sizeof(classes[0]); i++)
		printf("  %s = %p\n", ((struct ClassObjectData *) classes[i]->data)->name, (void *) classes[i]);
	*/

	return STATUS_OK;
}


void builtins_teardown(struct Interpreter *interp)
{
	if (interp->astnodeclass) {
		OBJECT_DECREF(interp, interp->astnodeclass);
		interp->astnodeclass = NULL;
	}

	if (interp->functionclass) {
		OBJECT_DECREF(interp, interp->functionclass);
		interp->functionclass = NULL;    // who needs this anyway?
	}

	if (interp->nomemerr) {
		OBJECT_DECREF(interp, interp->nomemerr);
		interp->nomemerr = NULL;
	}

	if (interp->classclass) {
		OBJECT_DECREF(interp, interp->classclass);
		interp->classclass = NULL;
	}

	if (interp->builtinctx) {
		context_free(interp->builtinctx);
		interp->builtinctx = NULL;
	}
}
