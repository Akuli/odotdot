#include "builtins.h"
#include <assert.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include "ast.h"
#include "builtins/arrayfunc.h"
#include "builtins/catch.h"
#include "builtins/operators.h"
#include "builtins/print.h"
#include "common.h"
#include "interpreter.h"
#include "objectsystem.h"
#include "objects/array.h"
#include "objects/block.h"
#include "objects/classobject.h"
#include "objects/errors.h"
#include "objects/function.h"
#include "objects/integer.h"
#include "objects/mapping.h"
#include "objects/object.h"
#include "objects/scope.h"
#include "objects/string.h"
#include "unicode.h"


static int create_method_mapping(struct Interpreter *interp, struct Object *klass)
{
	struct ClassObjectData *data = klass->data;
	assert(!data->methods);
	return (data->methods = mappingobject_newempty(interp)) ? STATUS_OK : STATUS_ERROR;
}

int builtins_setup(struct Interpreter *interp)
{
	if (!(interp->builtins.objectclass = objectobject_createclass_noerr(interp))) return STATUS_NOMEM;
	if (!(interp->builtins.classclass = classobject_create_classclass_noerr(interp))) return STATUS_NOMEM;

	interp->builtins.objectclass->klass = interp->builtins.classclass;
	OBJECT_INCREF(interp, interp->builtins.classclass);
	interp->builtins.classclass->klass = interp->builtins.classclass;
	OBJECT_INCREF(interp, interp->builtins.classclass);

	if (!(interp->builtins.stringclass = stringobject_createclass_noerr(interp))) return STATUS_NOMEM;
	if (!(interp->builtins.errorclass = errorobject_createclass_noerr(interp))) return STATUS_NOMEM;
	if (!(interp->builtins.nomemerr = errorobject_createnomemerr_noerr(interp))) return STATUS_NOMEM;

	// now interp->err stuff works
	// but note that error printing must NOT use any methods because methods don't actually exist yet
	if (!(interp->builtins.functionclass = functionobject_createclass(interp))) goto error;
	if (!(interp->builtins.mappingclass = mappingobject_createclass(interp))) goto error;

	// these classes must exist before methods exist, so they are handled specially
	// TODO: classclass
	if (create_method_mapping(interp, interp->builtins.classclass) == STATUS_ERROR) goto error;
	if (create_method_mapping(interp, interp->builtins.objectclass) == STATUS_ERROR) goto error;
	if (create_method_mapping(interp, interp->builtins.stringclass) == STATUS_ERROR) goto error;
	if (create_method_mapping(interp, interp->builtins.errorclass) == STATUS_ERROR) goto error;
	if (create_method_mapping(interp, interp->builtins.mappingclass) == STATUS_ERROR) goto error;
	if (create_method_mapping(interp, interp->builtins.functionclass) == STATUS_ERROR) goto error;
	if (classobject_addmethods(interp) == STATUS_ERROR) goto error;
	if (objectobject_addmethods(interp) == STATUS_ERROR) goto error;
	if (stringobject_addmethods(interp) == STATUS_ERROR) goto error;
	if (errorobject_addmethods(interp) == STATUS_ERROR) goto error;
	if (mappingobject_addmethods(interp) == STATUS_ERROR) goto error;
	if (functionobject_addmethods(interp) == STATUS_ERROR) goto error;

	if (!(interp->builtins.arrayclass = arrayobject_createclass(interp))) goto error;
	if (!(interp->builtins.integerclass = integerobject_createclass(interp))) goto error;
	if (!(interp->builtins.astnodeclass = astnode_createclass(interp))) goto error;
	if (!(interp->builtins.scopeclass = scopeobject_createclass(interp))) goto error;
	if (!(interp->builtins.blockclass = blockobject_createclass(interp))) goto error;

	if (!(interp->builtins.array_func = functionobject_new(interp, builtin_arrayfunc))) goto error;
	if (!(interp->builtins.catch = functionobject_new(interp, builtin_catch))) goto error;
	if (!(interp->builtins.equals = functionobject_new(interp, builtin_equals))) goto error;
	if (!(interp->builtins.print = functionobject_new(interp, builtin_print))) goto error;
	if (!(interp->builtins.same_object = functionobject_new(interp, builtin_sameobject))) goto error;

	if (!(interp->builtinscope = scopeobject_newbuiltin(interp))) goto error;

	if (interpreter_addbuiltin(interp, "Array", interp->builtins.arrayclass) == STATUS_ERROR) goto error;
	if (interpreter_addbuiltin(interp, "Error", interp->builtins.errorclass) == STATUS_ERROR) goto error;
	if (interpreter_addbuiltin(interp, "Integer", interp->builtins.integerclass) == STATUS_ERROR) goto error;
	if (interpreter_addbuiltin(interp, "Mapping", interp->builtins.mappingclass) == STATUS_ERROR) goto error;
	if (interpreter_addbuiltin(interp, "Object", interp->builtins.objectclass) == STATUS_ERROR) goto error;
	if (interpreter_addbuiltin(interp, "String", interp->builtins.stringclass) == STATUS_ERROR) goto error;
	if (interpreter_addbuiltin(interp, "array_func", interp->builtins.array_func) == STATUS_ERROR) goto error;
	if (interpreter_addbuiltin(interp, "catch", interp->builtins.catch) == STATUS_ERROR) goto error;
	if (interpreter_addbuiltin(interp, "equals", interp->builtins.equals) == STATUS_ERROR) goto error;
	if (interpreter_addbuiltin(interp, "print", interp->builtins.print) == STATUS_ERROR) goto error;
	if (interpreter_addbuiltin(interp, "same_object", interp->builtins.same_object) == STATUS_ERROR) goto error;

#ifdef DEBUG_BUILTINS       // compile like this:   $ CFLAGS=-DDEBUG_BUILTINS make clean all
	printf("things created by builtins_setup():\n");
#define debug(x) printf("  interp->%s = %p\n", #x, (void *) interp->x);
	debug(builtins.arrayclass);
	debug(builtins.astnodeclass);
	debug(builtins.blockclass);
	debug(builtins.classclass);
	debug(builtins.errorclass);
	debug(builtins.functionclass);
	debug(builtins.integerclass);
	debug(builtins.mappingclass);
	debug(builtins.objectclass);
	debug(builtins.scopeclass);
	debug(builtins.stringclass);

	debug(builtins.array_func);
	debug(builtins.nomemerr);
	debug(builtins.print);

	debug(builtinscope);
#undef debug
#endif   // DEBUG_BUILTINS

	assert(!(interp->err));
	return STATUS_OK;

error:
	fprintf(stderr, "an error occurred :(\n");    // TODO: better error message printing!
	if (interp->builtins.print) {
		struct Object *err = interp->err;
		interp->err = NULL;
		struct Object *printres = functionobject_call(interp, interp->builtins.print, (struct Object *) err->data, NULL);
		OBJECT_DECREF(interp, err);
		if (printres)
			OBJECT_DECREF(interp, printres);
		else     // print failed, interp->err is decreffed below
			OBJECT_DECREF(interp, interp->err);
	}

	OBJECT_DECREF(interp, interp->err);
	interp->err = NULL;
	return STATUS_ERROR;
}


void builtins_teardown(struct Interpreter *interp)
{
#define TEARDOWN(x) if (interp->builtins.x) { OBJECT_DECREF(interp, interp->builtins.x); interp->builtins.x = NULL; }
	TEARDOWN(arrayclass);
	TEARDOWN(astnodeclass);
	TEARDOWN(blockclass);
	TEARDOWN(classclass);
	TEARDOWN(errorclass);
	TEARDOWN(functionclass);
	TEARDOWN(integerclass);
	TEARDOWN(mappingclass);
	TEARDOWN(objectclass);
	TEARDOWN(scopeclass);
	TEARDOWN(stringclass);

	TEARDOWN(array_func);
	TEARDOWN(catch);
	TEARDOWN(equals);
	TEARDOWN(nomemerr);
	TEARDOWN(print);
	TEARDOWN(same_object);
#undef TEARDOWN

	if (interp->builtinscope) {
		OBJECT_DECREF(interp, interp->builtinscope);
		interp->builtinscope = NULL;
	}
}
