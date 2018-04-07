#include "builtins.h"
#include <assert.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
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
	struct Object *stringclass = interpreter_getbuiltin(ctx->interp, errptr, "String");
	if (!stringclass)    // errptr is set
		return NULL;

	// TODO: call to_string() and check arguments with errptr instead of assert
	assert(nargs == 1);
	assert(args[0]->klass == stringclass);
	OBJECT_DECREF(ctx->interp, stringclass);

	char *utf8;
	size_t utf8len;
	char errormsg[100];
	int status = utf8_encode(*((struct UnicodeString *) args[0]->data), &utf8, &utf8len, errormsg);
	if (status == STATUS_NOMEM) {
		*errptr = ctx->interp->nomemerr;
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

int builtins_setup(struct Interpreter *interp, struct Object **errptr)
{
	int status = STATUS_ERROR;   // only used on error
	struct ObjectClassInfo *objectinfo = NULL, *errorinfo = NULL, *stringinfo = NULL;
	struct Object *objectclass = NULL, *errorclass = NULL, *stringclass = NULL;
	struct Object *printfunc = NULL;

	if (!(objectinfo = objectobject_createclass())) goto nomem;
	if (!(errorinfo = errorobject_createclass(objectinfo))) goto nomem;
	if (!(stringinfo = stringobject_createclass(objectinfo))) goto nomem;
	if (!(interp->nomemerr = errorobject_createnomemerr(interp, errorinfo, stringinfo))) goto nomem;
	// now we can use errptr

	if (classobject_createclass(interp, errptr, objectinfo) == STATUS_ERROR) goto error;

	if (!(objectclass = classobject_newfromclassinfo(interp, errptr, objectinfo))) goto error;
	if (!(errorclass = classobject_newfromclassinfo(interp, errptr, errorinfo))) goto error;
	if (!(stringclass = classobject_newfromclassinfo(interp, errptr, stringinfo))) goto error;

	if (interpreter_addbuiltin(interp, errptr, "Object", objectclass) == STATUS_ERROR) goto error;
	if (interpreter_addbuiltin(interp, errptr, "Error", errorclass) == STATUS_ERROR) goto error;
	if (interpreter_addbuiltin(interp, errptr, "String", stringclass) == STATUS_ERROR) goto error;

	// yes, this is the best way to do this i could think of
	((struct Object *) interp->nomemerr->data)->klass = stringclass;
	OBJECT_INCREF(interp, stringclass);
	interp->nomemerr->klass = errorclass;
	OBJECT_INCREF(interp, errorclass);

	if (functionobject_createclass(interp, errptr) == STATUS_ERROR) goto error;
	if (arrayobject_createclass(interp, errptr) == STATUS_ERROR) goto error;
	if (integerobject_createclass(interp, errptr) == STATUS_ERROR) goto error;

	if (stringobject_addmethods(interp, errptr) == STATUS_ERROR) goto error;

	printfunc = functionobject_new(interp, errptr, print_builtin, NULL);
	if (!printfunc) goto error;
	if (interpreter_addbuiltin(interp, errptr, "print", printfunc) == STATUS_ERROR) goto error;

	// release refs from creating the objects, now the context holds references to these
	OBJECT_DECREF(interp, objectclass);
	OBJECT_DECREF(interp, errorclass);
	OBJECT_DECREF(interp, stringclass);
	OBJECT_DECREF(interp, printfunc);

	return STATUS_OK;

nomem:
	status = STATUS_NOMEM;
	// "fall through" to error

error:
	// FIXME: this might be very broken, it's hard to test this code
	if (status == STATUS_ERROR) {
		assert(interp->nomemerr);
		assert(*errptr == interp->nomemerr);
		// TODO: decreff *errptr if it will be increffed everywhere some day?
		status = STATUS_NOMEM;
	}

	if (printfunc) OBJECT_DECREF(interp, printfunc);
	if (interp->functionclass) OBJECT_DECREF(interp, interp->functionclass);
	if (interp->nomemerr)	OBJECT_DECREF(interp, interp->nomemerr);

	if (stringclass) OBJECT_DECREF(interp, stringclass);
	else if (stringinfo) objectclassinfo_free(interp, stringinfo);
	if (errorclass) OBJECT_DECREF(interp, errorclass);
	else if (errorinfo) objectclassinfo_free(interp, errorinfo);
	if (objectclass) OBJECT_DECREF(interp, errorclass);
	else if (objectinfo) objectclassinfo_free(interp, objectinfo);

	if (interp->classclass) OBJECT_DECREF(interp, interp->classclass);

	assert(status == STATUS_ERROR || status == STATUS_NOMEM);
	return status;
}


// TODO: is this too copy/pasta?
void builtins_teardown(struct Interpreter *interp)
{
	OBJECT_DECREF(interp, interp->nomemerr);

	// TODO: how about all sub contexts? this assumes that there are none
	context_free(interp->builtinctx);

	OBJECT_DECREF(interp, interp->functionclass);
	OBJECT_DECREF(interp, interp->classclass);

	/* this is a bit tricky because interp->classclass->klass == interp->classclass
	OBJECT_DECREF(interp->classclass) doesn't work because OBJECT_DECREF also decrefs ->klass
	this is one of the reasons why ->klass can be NULL and OBJECT_DECREF will ignore it

	TODO: implement gc nicely and use it instead
	*/
	struct ObjectClassInfo *info = interp->classclass->data;
	objectclassinfo_free(interp, info);
	interp->classclass->klass = NULL;
	OBJECT_DECREF(interp, interp->classclass);
}
