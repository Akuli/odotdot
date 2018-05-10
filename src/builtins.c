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


static struct Object *print_builtin(struct Context *ctx, struct Object **errptr, struct Object **args, size_t nargs)
{
	if (functionobject_checktypes(ctx, errptr, args, nargs, "String", NULL) == STATUS_ERROR)
		return NULL;

	char *utf8;
	size_t utf8len;
	char errormsg[100];
	int status = utf8_encode(*((struct UnicodeString *) args[0]->data), &utf8, &utf8len, errormsg);
	if (status == STATUS_NOMEM) {
		errorobject_setnomem(ctx->interp, errptr);
		return NULL;
	}
	assert(status == STATUS_OK);  // TODO: how about invalid unicode strings? make sure they don't exist when creating strings?

	// TODO: avoid writing 1 byte at a time... seems to be hard with c \0 strings
	for (size_t i=0; i < utf8len; i++)
		putchar(utf8[i]);
	free(utf8);
	putchar('\n');

	// this must return a new reference on success
	return stringobject_newfromcharptr(ctx->interp, errptr, "asd");
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

	// create some fun reference cycles
	// at this point, we know that everything will succeed
	objectclass->klass = interp->classclass;
	OBJECT_INCREF(interp, interp->classclass);
	interp->classclass->klass = interp->classclass;
	OBJECT_INCREF(interp, interp->classclass);

	// now the hard stuff is done \o/ let's do some easier things

	interp->functionclass = functionobject_createclass(interp, &err);
	if (!interp->functionclass) {
		fprintf(stderr, "an error occurred :(\n");    // TODO: better error message printing!
		OBJECT_DECREF(interp, err);
		return STATUS_ERROR;
	}

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


/* reference diagram:

    objectclass->klass        classclass->klass
  ,--------------------.    ,-------------------.
  |                    V    V                   |
objectclass  <-----  classclass  ---------------'
                     /|\ /|\ /|\
                      |   |   |
       ,--------------'   |   `-----------.
       |                  |               |
     other           stringclass  <--  errorclass
     stuff

the cycles must be deleted
*/

void builtins_teardown(struct Interpreter *interp)
{
	struct Object *objectclass = interpreter_getbuiltin_nomalloc(interp, "Object");

	if (interp->astnodeclass) {
		OBJECT_DECREF(interp, interp->astnodeclass);
		interp->astnodeclass = NULL;
	}

	if (interp->functionclass) {
		OBJECT_DECREF(interp, interp->functionclass);
		//interp->functionclass = NULL;    // who needs this anyway?
	}

	if (interp->nomemerr) {
		OBJECT_DECREF(interp, interp->nomemerr);
		interp->nomemerr = NULL;
	}

	// TODO: how about all sub contexts? this assumes that there are none
	context_free(interp->builtinctx);
	interp->builtinctx = NULL;

	// delete the fun reference cycles
	if (interp->classclass->klass) {
		assert(objectclass);    // if this fails, builtins_setup() is very broken or someone deleted Object
		assert(objectclass->klass == interp->classclass);
		assert(interp->classclass->klass == interp->classclass);

		// classobject_destructor() is not called automagically if ->klass == NULL
		// TODO: how about foreachref???
		objectclass->klass = NULL;
		OBJECT_DECREF(interp, interp->classclass);
		interp->classclass->klass = NULL;
		classobject_destructor(interp->classclass);
		OBJECT_DECREF(interp, interp->classclass);

		// classobject_create_classclass() increffed objectclass, but that didn't get
		// decreffed because classclass->klass was set to NULL before decreffing
		// classclass, and object_free_impl() didn't know anything about the reference
		OBJECT_DECREF(interp, objectclass);

		// note the interpreter_getbuiltin_nomalloc() above
		classobject_destructor(objectclass);
		OBJECT_DECREF(interp, objectclass);
	} else {
		assert(!objectclass);
	}

	if (interp->classclass) {
		OBJECT_DECREF(interp, interp->classclass);
		interp->classclass = NULL;
	}
}
