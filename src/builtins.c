#include "builtins.h"
#include <assert.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include "attribute.h"
#include "check.h"
#include "equals.h"
#include "interpreter.h"
#include "lambdabuiltin.h"
#include "method.h"
#include "objectsystem.h"
#include "objects/array.h"
#include "objects/astnode.h"
#include "objects/block.h"
#include "objects/classobject.h"
#include "objects/errors.h"
#include "objects/function.h"
#include "objects/integer.h"
#include "objects/mapping.h"
#include "objects/null.h"
#include "objects/object.h"
#include "objects/scope.h"
#include "objects/string.h"
#include "unicode.h"


static struct Object *subscope_of_defscope(struct Interpreter *interp, struct Object *block)
{
	struct Object *defscope = attribute_get(interp, block, "definition_scope");
	if (!defscope)
		return NULL;

	struct Object *subscope = scopeobject_newsub(interp, defscope);
	OBJECT_DECREF(interp, defscope);
	return subscope;
}

// FIXME: avoid allocating more memory in this, i think that makes this slow
static struct Object *if_(struct Interpreter *interp, struct Object *args, struct Object *opts)
{
	// bool is created in builtins.ö
	struct Object *boolclass = interpreter_getbuiltin(interp, "Bool");
	if (!boolclass)
		return NULL;

	bool ok = check_args(interp, args, boolclass, interp->builtins.Block, NULL);
	OBJECT_DECREF(interp, boolclass);
	if (!ok) return NULL;
	if (!check_opts(interp, opts, "else", interp->builtins.Block, NULL)) return NULL;

	struct Object *cond = ARRAYOBJECT_GET(args, 0);
	struct Object *ifblock = ARRAYOBJECT_GET(args, 1);

	// TODO: create a utility function that does this?
	struct Object *elsestr = stringobject_newfromcharptr(interp, "else");
	if (!elsestr)
		return NULL;
	struct Object *elseblock = NULL;
	int status = mappingobject_get(interp, opts, elsestr, &elseblock);
	OBJECT_DECREF(interp, elsestr);
	if (status == -1)
		return NULL;

	struct Object *truee = interpreter_getbuiltin(interp, "true");
	if (!truee) {
		if (elseblock)
			OBJECT_DECREF(interp, elseblock);
		return NULL;
	}

	bool doit = (cond == truee);
	OBJECT_DECREF(interp, truee);

	if (doit) {
		struct Object *scope = subscope_of_defscope(interp, ifblock);
		if (!scope)
			goto error;
		bool ok = blockobject_run(interp, ifblock, scope);
		OBJECT_DECREF(interp, scope);
		if (!ok)
			goto error;
	} else if (elseblock) {
		struct Object *scope = subscope_of_defscope(interp, elseblock);
		if (!scope)
			goto error;
		bool ok = blockobject_run(interp, elseblock, scope);
		OBJECT_DECREF(interp, scope);
		if (!ok)
			goto error;
	}

	if (elseblock)
		OBJECT_DECREF(interp, elseblock);
	return nullobject_get(interp);

error:
	if (elseblock)
		OBJECT_DECREF(interp, elseblock);
	return NULL;
}

// catch { ... } SomeError "varname" { ... };
static struct Object *catch(struct Interpreter *interp, struct Object *args, struct Object *opts)
{
	if (!check_args(interp, args, interp->builtins.Block, interp->builtins.Class, interp->builtins.String, interp->builtins.Block, NULL)) return NULL;
	if (!check_no_opts(interp, opts)) return NULL;   // TODO: finally option?
	struct Object *trying = ARRAYOBJECT_GET(args, 0);
	struct Object *errclass = ARRAYOBJECT_GET(args, 1);
	struct Object *varname = ARRAYOBJECT_GET(args, 2);
	struct Object *caught = ARRAYOBJECT_GET(args, 3);

	if (!classobject_issubclassof(errclass, interp->builtins.Error)) {
		errorobject_setwithfmt(interp, "TypeError", "cannot catch %U, it doesn't inherit from Error", ((struct ClassObjectData*)errclass->data)->name);
		return NULL;
	}

	struct Object *scope = subscope_of_defscope(interp, trying);
	bool ok = blockobject_run(interp, trying, scope);
	OBJECT_DECREF(interp, scope);
	if (ok)
		return nullobject_get(interp);

	if (!classobject_isinstanceof(interp->err, errclass))
		return NULL;

	assert(interp->err);
	struct Object *err = interp->err;
	interp->err = NULL;

	scope = subscope_of_defscope(interp, caught);
	if (!scope) {
		OBJECT_DECREF(interp, err);
		return NULL;
	}

	struct Object *localvars = attribute_get(interp, scope, "local_vars");
	if (!localvars) {
		OBJECT_DECREF(interp, scope);
		OBJECT_DECREF(interp, err);
		return NULL;
	}

	ok = mappingobject_set(interp, localvars, varname, err);
	OBJECT_DECREF(interp, err);
	OBJECT_DECREF(interp, localvars);
	if (!ok) {
		OBJECT_DECREF(interp, scope);
		return NULL;
	}

	ok = blockobject_run(interp, caught, scope);
	OBJECT_DECREF(interp, scope);
	return ok ? nullobject_get(interp) : NULL;
}


static struct Object *get_class(struct Interpreter *interp, struct Object *args, struct Object *opts)
{
	if (!check_args(interp, args, interp->builtins.Object, NULL)) return NULL;
	if (!check_no_opts(interp, opts)) return NULL;

	struct Object *obj = ARRAYOBJECT_GET(args, 0);
	OBJECT_INCREF(interp, obj->klass);
	return obj->klass;
}

#define BOOL(interp, x) interpreter_getbuiltin((interp), (x) ? "true" : "false")
static struct Object *is_instance_of(struct Interpreter *interp, struct Object *args, struct Object *opts)
{
	// TODO: shouldn't this be implemented in builtins.ö? classobject_isinstanceof() doesn't do anything fancy
	if (!check_args(interp, args, interp->builtins.Object, interp->builtins.Class, NULL)) return NULL;
	if (!check_no_opts(interp, opts)) return NULL;
	return BOOL(interp, classobject_isinstanceof(ARRAYOBJECT_GET(args, 0), ARRAYOBJECT_GET(args, 1)));
}

struct Object *equals_builtin(struct Interpreter *interp, struct Object *args, struct Object *opts)
{
	if (!check_args(interp, args, interp->builtins.Object, interp->builtins.Object, NULL)) return NULL;
	if (!check_no_opts(interp, opts)) return NULL;

	int res = equals(interp, ARRAYOBJECT_GET(args, 0), ARRAYOBJECT_GET(args, 1));
	if (res == -1)
		return NULL;

	assert(res == !!res);
	return BOOL(interp, res);
}

static struct Object *same_object(struct Interpreter *interp, struct Object *args, struct Object *opts)
{
	if (!check_args(interp, args, interp->builtins.Object, interp->builtins.Object, NULL)) return NULL;
	if (!check_no_opts(interp, opts)) return NULL;
	return BOOL(interp, ARRAYOBJECT_GET(args, 0) == ARRAYOBJECT_GET(args, 1));
}
#undef BOOL


static struct Object *print(struct Interpreter *interp, struct Object *args, struct Object *opts)
{
	if (!check_args(interp, args, interp->builtins.String, NULL)) return NULL;
	if (!check_no_opts(interp, opts)) return NULL;

	char *utf8;
	size_t utf8len;
	if (!utf8_encode(interp, *((struct UnicodeString *) ARRAYOBJECT_GET(args, 0)->data), &utf8, &utf8len))
		return NULL;

	// TODO: avoid writing 1 byte at a time... seems to be hard with c \0 strings
	for (size_t i=0; i < utf8len; i++)
		putchar(utf8[i]);
	free(utf8);
	putchar('\n');

	return nullobject_get(interp);
}


static struct Object *new(struct Interpreter *interp, struct Object *args, struct Object *opts)
{
	if (ARRAYOBJECT_LEN(args) == 0) {
		errorobject_setwithfmt(interp, "ArgError", "new needs at least 1 argument, the class");
		return NULL;
	}
	if (!check_type(interp, interp->builtins.Class, ARRAYOBJECT_GET(args, 0)))
		return NULL;
	struct Object *klass = ARRAYOBJECT_GET(args, 0);

	struct Object *obj = classobject_newinstance(interp, klass, NULL, NULL);
	if (!obj)
		return NULL;

	struct Object *setupargs = arrayobject_slice(interp, args, 1, ARRAYOBJECT_LEN(args));
	if (!setupargs) {
		OBJECT_DECREF(interp, obj);
		return NULL;
	}

	// there's no argument array taking version of method_call()
	struct Object *setup = attribute_get(interp, obj, "setup");
	if (!setup) {
		OBJECT_DECREF(interp, setupargs);
		OBJECT_DECREF(interp, obj);
		return NULL;
	}

	struct Object *res = functionobject_vcall(interp, setup, setupargs, opts);
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


// every objects may have an attrdata mapping, values of simple attributes go there
// attrdata is first set to NULL and created when needed
// see also definition of struct Object
static struct Object *get_attrdata(struct Interpreter *interp, struct Object *args, struct Object *opts)
{
	if (!check_args(interp, args, interp->builtins.Object, NULL)) return NULL;
	if (!check_no_opts(interp, opts)) return NULL;

	struct Object *obj = ARRAYOBJECT_GET(args, 0);
	if (!obj->attrdata) {
		obj->attrdata = mappingobject_newempty(interp);
		if (!obj->attrdata)
			return NULL;
	}

	OBJECT_INCREF(interp, obj->attrdata);
	return obj->attrdata;
}


static bool add_function(struct Interpreter *interp, char *name, functionobject_cfunc cfunc)
{
	struct Object *func = functionobject_new(interp, cfunc, name);
	if (!func)
		return false;

	bool ok = interpreter_addbuiltin(interp, name, func);
	OBJECT_DECREF(interp, func);
	return ok;
}

bool builtins_setup(struct Interpreter *interp)
{
	if (!(interp->builtins.Object = objectobject_createclass_noerr(interp))) goto nomem;
	if (!(interp->builtins.Class = classobject_create_Class_noerr(interp))) goto nomem;

	interp->builtins.Object->klass = interp->builtins.Class;
	OBJECT_INCREF(interp, interp->builtins.Class);
	interp->builtins.Class->klass = interp->builtins.Class;
	OBJECT_INCREF(interp, interp->builtins.Class);

	if (!(interp->builtins.String = stringobject_createclass_noerr(interp))) goto nomem;
	if (!(interp->builtins.Error = errorobject_createclass_noerr(interp))) goto nomem;
	if (!(interp->builtins.nomemerr = errorobject_createnomemerr_noerr(interp))) goto nomem;

	// now interp->err stuff works
	// but note that error printing must NOT use any methods because methods don't actually exist yet
	if (!(interp->builtins.Function = functionobject_createclass(interp))) goto error;
	if (!(interp->builtins.Mapping = mappingobject_createclass(interp))) goto error;

	// these classes must exist before methods exist, so they are handled specially
	// TODO: rename addmethods to addattributes functions? methods are attributes
	if (!classobject_addmethods(interp)) goto error;
	if (!objectobject_addmethods(interp)) goto error;
	if (!stringobject_addmethods(interp)) goto error;
	if (!errorobject_addmethods(interp)) goto error;
	if (!mappingobject_addmethods(interp)) goto error;
	if (!functionobject_addmethods(interp)) goto error;

	if (!classobject_setname(interp, interp->builtins.Class, "Class")) goto error;
	if (!classobject_setname(interp, interp->builtins.Object, "Object")) goto error;
	if (!classobject_setname(interp, interp->builtins.String, "String")) goto error;
	if (!classobject_setname(interp, interp->builtins.Mapping, "Mapping")) goto error;
	if (!classobject_setname(interp, interp->builtins.Function, "Function")) goto error;

	if (!classobject_setname(interp, interp->builtins.Error, "Error")) goto error;
	if (!classobject_setname(interp, interp->builtins.nomemerr->klass, "MemError")) goto error;

	if (!(interp->builtins.null = nullobject_create(interp))) goto error;
	if (!(interp->builtins.Array = arrayobject_createclass(interp))) goto error;
	if (!(interp->builtins.Integer = integerobject_createclass(interp))) goto error;
	if (!(interp->builtins.AstNode = astnodeobject_createclass(interp))) goto error;
	if (!(interp->builtins.Scope = scopeobject_createclass(interp))) goto error;
	if (!(interp->builtins.Block = blockobject_createclass(interp))) goto error;

	if (!(interp->builtinscope = scopeobject_newbuiltin(interp))) goto error;

	if (!interpreter_addbuiltin(interp, "Array", interp->builtins.Array)) goto error;
	if (!interpreter_addbuiltin(interp, "Block", interp->builtins.Block)) goto error;
	if (!interpreter_addbuiltin(interp, "Integer", interp->builtins.Integer)) goto error;
	if (!interpreter_addbuiltin(interp, "Mapping", interp->builtins.Mapping)) goto error;
	if (!interpreter_addbuiltin(interp, "Object", interp->builtins.Object)) goto error;
	if (!interpreter_addbuiltin(interp, "Scope", interp->builtins.Scope)) goto error;
	if (!interpreter_addbuiltin(interp, "String", interp->builtins.String)) goto error;
	if (!interpreter_addbuiltin(interp, "null", interp->builtins.null)) goto error;

	if (!interpreter_addbuiltin(interp, "Error", interp->builtins.Error)) goto error;
	if (!interpreter_addbuiltin(interp, "MemError", interp->builtins.nomemerr->klass)) goto error;

	if (!add_function(interp, "if", if_)) goto error;
	if (!add_function(interp, "lambda", lambdabuiltin)) goto error;
	if (!add_function(interp, "catch", catch)) goto error;
	if (!add_function(interp, "equals", equals_builtin)) goto error;
	if (!add_function(interp, "get_class", get_class)) goto error;
	if (!add_function(interp, "is_instance_of", is_instance_of)) goto error;
	if (!add_function(interp, "new", new)) goto error;
	if (!add_function(interp, "print", print)) goto error;
	if (!add_function(interp, "same_object", same_object)) goto error;
	if (!add_function(interp, "get_attrdata", get_attrdata)) goto error;

	// compile like this:   $ CFLAGS=-DDEBUG_BUILTINS make clean all
#ifdef DEBUG_BUILTINS
	printf("things created by builtins_setup():\n");
#define debug(x) printf("  interp->%s = %p\n", #x, (void *) interp->x);
	debug(builtins.Array);
	debug(builtins.AstNode);
	debug(builtins.Block);
	debug(builtins.Class);
	debug(builtins.Error);
	debug(builtins.Function);
	debug(builtins.Integer);
	debug(builtins.Mapping);
	debug(builtins.Object);
	debug(builtins.Scope);
	debug(builtins.String);

	debug(builtins.nomemerr);
	debug(builtinscope);
#undef debug
#endif   // DEBUG_BUILTINS

	assert(!(interp->err));
	return true;

error:
	fprintf(stderr, "an error occurred :(\n");    // TODO: better error message printing!
	assert(0);

	struct Object *print = interpreter_getbuiltin(interp, "print");
	if (print) {
		struct Object *err = interp->err;
		interp->err = NULL;
		struct Object *printres = functionobject_call(interp, print, (struct Object *) err->data, NULL);
		OBJECT_DECREF(interp, err);
		if (printres)
			OBJECT_DECREF(interp, printres);
		else     // print failed, interp->err is decreffed below
			OBJECT_DECREF(interp, interp->err);
		OBJECT_DECREF(interp, print);
	}

	OBJECT_DECREF(interp, interp->err);
	interp->err = NULL;
	return false;

nomem:
	fprintf(stderr, "%s: not enough memory for setting up builtins\n", interp->argv0);
	return false;
}


void builtins_teardown(struct Interpreter *interp)
{
#define TEARDOWN(x) if (interp->builtins.x) { OBJECT_DECREF(interp, interp->builtins.x); interp->builtins.x = NULL; }
	TEARDOWN(Array);
	TEARDOWN(AstNode);
	TEARDOWN(Block);
	TEARDOWN(Class);
	TEARDOWN(Error);
	TEARDOWN(Function);
	TEARDOWN(Integer);
	TEARDOWN(Mapping);
	TEARDOWN(Object);
	TEARDOWN(Scope);
	TEARDOWN(String);
	TEARDOWN(null);
	TEARDOWN(nomemerr);
#undef TEARDOWN

	if (interp->builtinscope) {
		OBJECT_DECREF(interp, interp->builtinscope);
		interp->builtinscope = NULL;
	}
}
