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
#include "objects/classobject.h"
#include "objects/errors.h"
#include "objects/function.h"
#include "objects/object.h"
#include "objects/string.h"


static struct Object *print_builtin(struct Context *ctx, struct Object **errptr, struct Object **args, size_t nargs)
{
	struct Object *stringclass = interpreter_getbuiltin(ctx->interp, errptr, "String");
	if (!stringclass)    // errptr is set
		return NULL;

	// TODO: call to_string() and check arguments with errptr instead of assert
	assert(nargs == 1);
	assert(args[0]->klass == (struct ObjectClassInfo *) stringclass->data);
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

// this is ugly
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

	if (functionobject_createclass(interp, errptr) == STATUS_ERROR) goto error;

	printfunc = functionobject_new(interp, errptr, print_builtin);
	assert(printfunc->klass == interp->functionobjectinfo);
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

	// these undo the above stuff, in a reversed order
	// e.g. objectinfo is created first, so it's freed last
	if (printfunc) OBJECT_DECREF(interp, printfunc);

	if (interp->functionobjectinfo) objectclassinfo_free(interp->functionobjectinfo);

	if (stringclass) OBJECT_DECREF(interp, stringclass);
	if (errorclass) OBJECT_DECREF(interp, errorclass);
	if (objectclass) OBJECT_DECREF(interp, objectclass);

	if (interp->classobjectinfo) objectclassinfo_free(interp->classobjectinfo);

	if (interp->nomemerr)	OBJECT_DECREF(interp, interp->nomemerr);
	if (stringinfo) objectclassinfo_free(stringinfo);
	if (errorinfo) objectclassinfo_free(stringinfo);
	if (objectinfo) objectclassinfo_free(objectinfo);

	assert(status == STATUS_OK || status == STATUS_NOMEM);
	return status;
}


// TODO: is this too copy/pasta?
void builtins_teardown(struct Interpreter *interp)
{
	OBJECT_DECREF(interp, interp->nomemerr);

	struct Object *objectclass = interpreter_getbuiltin_nomalloc(interp, "Object");
	struct Object *errorclass = interpreter_getbuiltin_nomalloc(interp, "Error");
	struct Object *stringclass = interpreter_getbuiltin_nomalloc(interp, "String");
	struct ObjectClassInfo *objectinfo = objectclass ? objectclass->data : NULL;
	struct ObjectClassInfo *errorinfo = errorclass ? errorclass->data : NULL;
	struct ObjectClassInfo *stringinfo = stringclass ? stringclass->data : NULL;
	if (objectclass) OBJECT_DECREF(interp, objectclass);
	if (errorclass) OBJECT_DECREF(interp, errorclass);
	if (stringclass) OBJECT_DECREF(interp, stringclass);
	if (stringinfo) objectclassinfo_free(stringinfo);
	if (errorinfo) objectclassinfo_free(errorinfo);
	if (objectinfo) objectclassinfo_free(objectinfo);

	context_free(interp->builtinctx);   	// TODO: how about all sub contexts? assume there are none??
	objectclassinfo_free(interp->functionobjectinfo);
	objectclassinfo_free(interp->classobjectinfo);
}
