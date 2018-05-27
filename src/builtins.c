#include "builtins.h"
#include <assert.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include "ast.h"
#include "attribute.h"
#include "check.h"
#include "common.h"
#include "equals.h"
#include "interpreter.h"
#include "method.h"
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


// this is partialled to a block of code to create array_funcs
static struct Object *arrayfunc_runner(struct Interpreter *interp, struct Object *argarr)
{
	struct Object *parentscope = attribute_get(interp, ARRAYOBJECT_GET(argarr, 0), "definition_scope");
	if (!parentscope)
		return NULL;

	struct Object *scope = scopeobject_newsub(interp, parentscope);
	OBJECT_DECREF(interp, parentscope);
	if (!scope)
		return NULL;

	struct Object *localvars = NULL, *string = NULL, *array = NULL;
	if (!((localvars = attribute_get(interp, scope, "local_vars")) &&
			(string = stringobject_newfromcharptr(interp, "arguments")) &&
			(array = arrayobject_slice(interp, argarr, 1, ARRAYOBJECT_LEN(argarr))))) {
		if (localvars) OBJECT_DECREF(interp, localvars);
		if (string) OBJECT_DECREF(interp, string);
		if (array) OBJECT_DECREF(interp, array);
		OBJECT_DECREF(interp, scope);
		return NULL;
	}

	int status = mappingobject_set(interp, localvars, string, array);
	OBJECT_DECREF(interp, localvars);
	OBJECT_DECREF(interp, string);
	OBJECT_DECREF(interp, array);
	if (status == STATUS_ERROR) {
		OBJECT_DECREF(interp, scope);
		return NULL;
	}

	struct Object *res = method_call(interp, ARRAYOBJECT_GET(argarr, 0), "run", scope, NULL);
	OBJECT_DECREF(interp, scope);
	return res;
}

static struct Object *array_func(struct Interpreter *interp, struct Object *argarr)
{
	if (check_args(interp, argarr, interp->builtins.blockclass, NULL) == STATUS_ERROR)
		return NULL;
	struct Object *block = ARRAYOBJECT_GET(argarr, 0);

	struct Object *runnerobj = functionobject_new(interp, arrayfunc_runner);
	if (!runnerobj)
		return NULL;

	struct Object *res = functionobject_newpartial(interp, runnerobj, block);
	OBJECT_DECREF(interp, runnerobj);
	return res;
}


static int run_block(struct Interpreter *interp, struct Object *block)
{
	struct Object *defscope = attribute_get(interp, block, "definition_scope");
	if (!defscope)
		return STATUS_ERROR;

	struct Object *subscope = scopeobject_newsub(interp, defscope);
	OBJECT_DECREF(interp, defscope);
	if (!subscope)
		return STATUS_ERROR;

	struct Object *res = method_call(interp, block, "run", subscope, NULL);
	OBJECT_DECREF(interp, subscope);
	if (!res)
		return STATUS_ERROR;

	OBJECT_DECREF(interp, res);
	return STATUS_OK;
}

static struct Object *catch(struct Interpreter *interp, struct Object *argarr)
{
	if (check_args(interp, argarr, interp->builtins.blockclass, interp->builtins.blockclass, NULL) == STATUS_ERROR)
		return NULL;
	struct Object *trying = ARRAYOBJECT_GET(argarr, 0);
	struct Object *caught = ARRAYOBJECT_GET(argarr, 1);

	if (run_block(interp, trying) == STATUS_ERROR) {
		// TODO: make the error available somewhere instead of resetting it here?
		assert(interp->err);
		OBJECT_DECREF(interp, interp->err);
		interp->err = NULL;

		if (run_block(interp, caught) == STATUS_ERROR)
			return NULL;
	}

	// everything succeeded or the error handling code succeeded
	return interpreter_getbuiltin(interp, "null");
}


#define BOOL(interp, x) interpreter_getbuiltin((interp), (x) ? "true" : "false")
struct Object *equals_builtin(struct Interpreter *interp, struct Object *argarr)
{
	if (check_args(interp, argarr, interp->builtins.objectclass, interp->builtins.objectclass, NULL) == STATUS_ERROR)
		return NULL;

	int res = equals(interp, ARRAYOBJECT_GET(argarr, 0), ARRAYOBJECT_GET(argarr, 1));
	if (res == -1)
		return NULL;

	assert(res == !!res);
	return BOOL(interp, res);
}

static struct Object *same_object(struct Interpreter *interp, struct Object *argarr)
{
	if (check_args(interp, argarr, interp->builtins.objectclass, interp->builtins.objectclass, NULL) == STATUS_ERROR)
		return NULL;
	return BOOL(interp, ARRAYOBJECT_GET(argarr, 0) == ARRAYOBJECT_GET(argarr, 1));
}
#undef BOOL


static struct Object *print(struct Interpreter *interp, struct Object *argarr)
{
	if (check_args(interp, argarr, interp->builtins.stringclass, NULL) == STATUS_ERROR)
		return NULL;

	char *utf8;
	size_t utf8len;
	char errormsg[100];
	int status = utf8_encode(*((struct UnicodeString *) ARRAYOBJECT_GET(argarr, 0)->data), &utf8, &utf8len, errormsg);
	if (status == STATUS_NOMEM) {
		errorobject_setnomem(interp);
		return NULL;
	}
	assert(status == STATUS_OK);  // TODO: how about invalid unicode strings? make sure they don't exist when creating strings?

	// TODO: avoid writing 1 byte at a time... seems to be hard with c \0 strings
	for (size_t i=0; i < utf8len; i++)
		putchar(utf8[i]);
	free(utf8);
	putchar('\n');

	return interpreter_getbuiltin(interp, "null");
}


static struct Object *new(struct Interpreter *interp, struct Object *argarr)
{
	if (ARRAYOBJECT_LEN(argarr) == 0) {
		errorobject_setwithfmt(interp, "new needs at least 1 argument, the class");
		return NULL;
	}
	if (check_type(interp, interp->builtins.classclass, ARRAYOBJECT_GET(argarr, 0)) == STATUS_ERROR)
		return NULL;
	struct Object *klass = ARRAYOBJECT_GET(argarr, 0);

	struct Object *obj = classobject_newinstance(interp, klass, NULL, NULL);
	if (!obj)
		return NULL;

	struct Object *setupargs = arrayobject_slice(interp, argarr, 1, ARRAYOBJECT_LEN(argarr));
	if (!setupargs) {
		OBJECT_DECREF(interp, obj);
		return NULL;
	}

	// there's no argument array taking version of method_call()
	struct Object *setup = method_get(interp, obj, "setup");
	if (!setup) {
		OBJECT_DECREF(interp, setupargs);
		OBJECT_DECREF(interp, obj);
		return NULL;
	}

	struct Object *res = functionobject_vcall(interp, setup, setupargs);
	OBJECT_DECREF(interp, setupargs);
	OBJECT_DECREF(interp, setup);
	if (!res) {
		OBJECT_DECREF(interp, obj);
		return NULL;
	}
	OBJECT_DECREF(interp, res);

	// no need to incref, this thing is already holding a reference to obj
	return obj;
}


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

	if (!(interp->builtins.array_func = functionobject_new(interp, array_func))) goto error;
	if (!(interp->builtins.catch = functionobject_new(interp, catch))) goto error;
	if (!(interp->builtins.equals = functionobject_new(interp, equals_builtin))) goto error;
	if (!(interp->builtins.new = functionobject_new(interp, new))) goto error;
	if (!(interp->builtins.print = functionobject_new(interp, print))) goto error;
	if (!(interp->builtins.same_object = functionobject_new(interp, same_object))) goto error;

	if (!(interp->builtinscope = scopeobject_newbuiltin(interp))) goto error;

	if (interpreter_addbuiltin(interp, "Array", interp->builtins.arrayclass) == STATUS_ERROR) goto error;
	if (interpreter_addbuiltin(interp, "Error", interp->builtins.errorclass) == STATUS_ERROR) goto error;
	if (interpreter_addbuiltin(interp, "Integer", interp->builtins.integerclass) == STATUS_ERROR) goto error;
	if (interpreter_addbuiltin(interp, "Mapping", interp->builtins.mappingclass) == STATUS_ERROR) goto error;
	if (interpreter_addbuiltin(interp, "Object", interp->builtins.objectclass) == STATUS_ERROR) goto error;
	if (interpreter_addbuiltin(interp, "Scope", interp->builtins.scopeclass) == STATUS_ERROR) goto error;
	if (interpreter_addbuiltin(interp, "String", interp->builtins.stringclass) == STATUS_ERROR) goto error;
	if (interpreter_addbuiltin(interp, "array_func", interp->builtins.array_func) == STATUS_ERROR) goto error;
	if (interpreter_addbuiltin(interp, "catch", interp->builtins.catch) == STATUS_ERROR) goto error;
	if (interpreter_addbuiltin(interp, "equals", interp->builtins.equals) == STATUS_ERROR) goto error;
	if (interpreter_addbuiltin(interp, "new", interp->builtins.new) == STATUS_ERROR) goto error;
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
	debug(builtins.catch);
	debug(builtins.equals);
	debug(builtins.new);
	debug(builtins.nomemerr);
	debug(builtins.print);
	debug(builtins.same_object);

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
	TEARDOWN(new);
	TEARDOWN(nomemerr);
	TEARDOWN(print);
	TEARDOWN(same_object);
#undef TEARDOWN

	if (interp->builtinscope) {
		OBJECT_DECREF(interp, interp->builtinscope);
		interp->builtinscope = NULL;
	}
}
