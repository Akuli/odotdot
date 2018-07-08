#include "builtins.h"
#include <assert.h>
#include <errno.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "attribute.h"
#include "check.h"
#include "import.h"
#include "interpreter.h"
#include "lambdabuiltin.h"
#include "objectsystem.h"
#include "objects/array.h"
#include "objects/astnode.h"
#include "objects/block.h"
#include "objects/bool.h"
#include "objects/classobject.h"
#include "objects/errors.h"
#include "objects/function.h"
#include "objects/integer.h"
#include "objects/library.h"
#include "objects/mapping.h"
#include "objects/null.h"
#include "objects/object.h"
#include "objects/option.h"
#include "objects/scope.h"
#include "objects/stackframe.h"
#include "objects/string.h"
#include "utf8.h"


static struct Object *subscope_of_defscope(struct Interpreter *interp, struct Object *block)
{
	struct Object *defscope = attribute_get(interp, block, "definition_scope");
	if (!defscope)
		return NULL;

	struct Object *subscope = scopeobject_newsub(interp, defscope);
	OBJECT_DECREF(interp, defscope);
	return subscope;
}

static struct Object *if_(struct Interpreter *interp, struct ObjectData nulldata, struct Object *args, struct Object *opts)
{
	if (!check_args(interp, args, interp->builtins.Bool, interp->builtins.Block, NULL)) return NULL;
	if (!check_opts(interp, opts, interp->strings.else_, interp->builtins.Block, NULL)) return NULL;

	struct Object *cond = ARRAYOBJECT_GET(args, 0);
	struct Object *ifblock = ARRAYOBJECT_GET(args, 1);

	struct Object *elseblock = NULL;
	int status = mappingobject_get(interp, opts, interp->strings.else_, &elseblock);
	if (status == -1)
		return NULL;

	if (cond == interp->builtins.yes) {
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

// for { init; } { cond } { incr; } { ... };
// TODO: break and continue
static struct Object *for_(struct Interpreter *interp, struct ObjectData nulldata, struct Object *args, struct Object *opts)
{
	if (!check_args(interp, args, interp->builtins.Block, interp->builtins.Block, interp->builtins.Block, interp->builtins.Block, NULL)) return NULL;
	if (!check_no_opts(interp, opts)) return NULL;
	struct Object *init = ARRAYOBJECT_GET(args, 0);
	struct Object *cond = ARRAYOBJECT_GET(args, 1);
	struct Object *incr = ARRAYOBJECT_GET(args, 2);
	struct Object *body = ARRAYOBJECT_GET(args, 3);

	struct Object *scope = subscope_of_defscope(interp, body);
	if (!scope)
		return NULL;

	if (!blockobject_run(interp, init, scope)) {
		OBJECT_DECREF(interp, scope);
		return NULL;
	}

	while(1) {
		struct Object *keepgoing = blockobject_runwithreturn(interp, cond, scope);
		if (!keepgoing) {
			OBJECT_DECREF(interp, scope);
			return NULL;
		}

		bool go = keepgoing==interp->builtins.yes;
		bool omg = (keepgoing != interp->builtins.yes && keepgoing != interp->builtins.no);
		OBJECT_DECREF(interp, keepgoing);
		if (omg) {
			errorobject_throwfmt(interp, "TypeError", "the condition of a for loop must return true or false, but it returned %D", keepgoing);
			OBJECT_DECREF(interp, scope);
			return NULL;
		}
		if (!go)
			break;

		if (!blockobject_run(interp, body, scope)) {
			OBJECT_DECREF(interp, scope);
			return NULL;
		}
		if (!blockobject_run(interp, incr, scope)) {
			OBJECT_DECREF(interp, scope);
			return NULL;
		}
	}

	OBJECT_DECREF(interp, scope);
	return nullobject_get(interp);
}

static struct Object *throw(struct Interpreter *interp, struct ObjectData nulldata, struct Object *args, struct Object *opts)
{
	if (!check_args(interp, args, interp->builtins.Error, NULL)) return NULL;
	if (!check_no_opts(interp, opts)) return NULL;

	errorobject_throw(interp, ARRAYOBJECT_GET(args, 0));
	return NULL;
}

// catch { ... } errspec { ... };
// errspec can be an error class or an [errorclass varname] array
static struct Object *catch(struct Interpreter *interp, struct ObjectData nulldata, struct Object *args, struct Object *opts)
{
	if (!check_args(interp, args, interp->builtins.Block, interp->builtins.Object, interp->builtins.Block, NULL)) return NULL;
	if (!check_no_opts(interp, opts)) return NULL;   // TODO: finally option?
	struct Object *trying = ARRAYOBJECT_GET(args, 0);
	struct Object *errspec = ARRAYOBJECT_GET(args, 1);
	struct Object *caught = ARRAYOBJECT_GET(args, 2);

	struct Object *errclass, *varname;
	if (classobject_isinstanceof(errspec, interp->builtins.Class)) {
		errclass = errspec;
		varname = NULL;
	} else if (classobject_isinstanceof(errspec, interp->builtins.Array)) {
		if (ARRAYOBJECT_LEN(errspec) != 2) {
			errorobject_throwfmt(interp, "ValueError", "expected a pair like [errorclass varname], got %D", errspec);
			return NULL;
		}
		errclass = ARRAYOBJECT_GET(errspec, 0);
		varname = ARRAYOBJECT_GET(errspec, 1);
		if (!check_type(interp, interp->builtins.Class, errclass)) return NULL;
		if (!check_type(interp, interp->builtins.String, varname)) return NULL;
	} else {
		errorobject_throwfmt(interp, "TypeError", "expected a subclass of Error or an [errorclass varname] array, got %D", errspec);
		return NULL;
	}

	if (!classobject_issubclassof(errclass, interp->builtins.Error)) {
		errorobject_throwfmt(interp, "TypeError", "cannot catch %U, it doesn't inherit from Error", ((struct ClassObjectData*)errclass->objdata.data)->name);
		return NULL;
	}

	struct Object *scope = subscope_of_defscope(interp, trying);
	bool ok = blockobject_run(interp, trying, scope);
	OBJECT_DECREF(interp, scope);
	if (ok)
		return nullobject_get(interp);

	assert(interp->err);

	if (!classobject_isinstanceof(interp->err, errclass))
		return NULL;

	struct Object *err = interp->err;
	interp->err = NULL;

	scope = subscope_of_defscope(interp, caught);
	if (!scope) {
		OBJECT_DECREF(interp, err);
		return NULL;
	}

	if (varname) {
		bool ok = mappingobject_set(interp, SCOPEOBJECT_LOCALVARS(scope), varname, err);
		OBJECT_DECREF(interp, err);
		if (!ok) {
			OBJECT_DECREF(interp, scope);
			return NULL;
		}
	} else {
		OBJECT_DECREF(interp, err);
	}

	ok = blockobject_run(interp, caught, scope);
	OBJECT_DECREF(interp, scope);
	return ok ? nullobject_get(interp) : NULL;
}


static struct Object *get_class(struct Interpreter *interp, struct ObjectData nulldata, struct Object *args, struct Object *opts)
{
	if (!check_args(interp, args, interp->builtins.Object, NULL)) return NULL;
	if (!check_no_opts(interp, opts)) return NULL;

	struct Object *obj = ARRAYOBJECT_GET(args, 0);
	OBJECT_INCREF(interp, obj->klass);
	return obj->klass;
}

static struct Object *same_object(struct Interpreter *interp, struct ObjectData nulldata, struct Object *args, struct Object *opts)
{
	if (!check_args(interp, args, interp->builtins.Object, interp->builtins.Object, NULL)) return NULL;
	if (!check_no_opts(interp, opts)) return NULL;
	return boolobject_get(interp, ARRAYOBJECT_GET(args, 0) == ARRAYOBJECT_GET(args, 1));
}


// TODO: write a lib for io and implement print with it
static struct Object *print(struct Interpreter *interp, struct ObjectData nulldata, struct Object *args, struct Object *opts)
{
	if (!check_args(interp, args, interp->builtins.String, NULL)) return NULL;
	if (!check_no_opts(interp, opts)) return NULL;

	char *utf8;
	size_t utf8len;
	if (!utf8_encode(interp, *((struct UnicodeString *) ARRAYOBJECT_GET(args, 0)->objdata.data), &utf8, &utf8len))
		return NULL;

	// fwrite in c99: if size or nmemb is zero, fwrite returns zero
	// errno for fwrite and putchar probably doesn't work on windows because c99 says nothing about it
	errno = 0;
	bool ok = utf8len ? fwrite(utf8, utf8len, 1, stdout)>0 : true;
	free(utf8);
	if (ok)
		ok = (putchar('\n') != EOF);
	if (!ok) {
		if (errno == 0)
			errorobject_throwfmt(interp, "IoError", "printing failed");
		else
			errorobject_throwfmt(interp, "IoError", "printing failed: %s", strerror(errno));
		return NULL;
	}
	return nullobject_get(interp);
}


static struct Object *new(struct Interpreter *interp, struct ObjectData nulldata, struct Object *args, struct Object *opts)
{
	if (ARRAYOBJECT_LEN(args) == 0) {
		errorobject_throwfmt(interp, "ArgError", "new needs at least 1 argument, the class");
		return NULL;
	}
	if (!check_type(interp, interp->builtins.Class, ARRAYOBJECT_GET(args, 0)))
		return NULL;
	struct Object *klass = ARRAYOBJECT_GET(args, 0);

	struct Object *obj;

	struct ClassObjectData *data = klass->objdata.data;
	if (data->newinstance)
		obj = data->newinstance(interp, args, opts);
	else {
		obj = object_new_noerr(interp, ARRAYOBJECT_GET(args, 0), (struct ObjectData){.data=NULL, .foreachref=NULL, .destructor=NULL});
		if (!obj)
			errorobject_thrownomem(interp);
	}
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

	if (!check_type(interp, interp->builtins.Function, setup)) {
		OBJECT_DECREF(interp, setup);
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
static struct Object *get_attrdata(struct Interpreter *interp, struct ObjectData nulldata, struct Object *args, struct Object *opts)
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
	struct Object *func = functionobject_new(interp, (struct ObjectData){.data=NULL, .foreachref=NULL, .destructor=NULL}, cfunc, name);
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

	if (!(interp->builtins.Option = optionobject_createclass_noerr(interp))) goto nomem;
	if (!(interp->builtins.none = optionobject_createnone_noerr(interp))) goto nomem;
	if (!(interp->builtins.null = nullobject_create_noerr(interp))) goto nomem;
	if (!(interp->builtins.String = stringobject_createclass_noerr(interp))) goto nomem;
	if (!(interp->builtins.Error = errorobject_createclass_noerr(interp))) goto nomem;
	if (!(interp->builtins.nomemerr = errorobject_createnomemerr_noerr(interp))) goto nomem;

	// now interp->err stuff works
	// but note that error printing must not use any methods because methods don't actually exist yet
#define INIT_STRING(NAME, VALUE) if (!(interp->strings.NAME = stringobject_newfromcharptr(interp, VALUE))) goto error;
	INIT_STRING(else_, "else")
	INIT_STRING(empty, "")
	INIT_STRING(export, "export")
	INIT_STRING(return_, "return")
#undef INIT_STRING

	if (!(interp->builtins.Function = functionobject_createclass(interp))) goto error;
	if (!(interp->builtins.Mapping = mappingobject_createclass(interp))) goto error;

	// these classes must exist before methods exist, so they are handled specially
	// TODO: rename addmethods to addattrib(ute)s functions? methods are attributes
	if (!classobject_addmethods(interp)) goto error;
	if (!objectobject_addmethods(interp)) goto error;
	if (!stringobject_addmethods(interp)) goto error;
	if (!errorobject_addmethods(interp)) goto error;
	if (!mappingobject_addmethods(interp)) goto error;
	if (!functionobject_addmethods(interp)) goto error;
	if (!optionobject_addmethods(interp)) goto error;

	if (!classobject_setname(interp, interp->builtins.null->klass, "Null")) goto error;
	if (!classobject_setname(interp, interp->builtins.Class, "Class")) goto error;
	if (!classobject_setname(interp, interp->builtins.Object, "Object")) goto error;
	if (!classobject_setname(interp, interp->builtins.Option, "Option")) goto error;
	if (!classobject_setname(interp, interp->builtins.String, "String")) goto error;
	if (!classobject_setname(interp, interp->builtins.Mapping, "Mapping")) goto error;
	if (!classobject_setname(interp, interp->builtins.Function, "Function")) goto error;
	if (!classobject_setname(interp, interp->builtins.Error, "Error")) goto error;
	if (!classobject_setname(interp, interp->builtins.nomemerr->klass, "MemError")) goto error;

	if (!boolobject_create(interp, &interp->builtins.yes, &interp->builtins.no)) goto error;
	interp->builtins.Bool = interp->builtins.no->klass;
	OBJECT_INCREF(interp, interp->builtins.Bool);

	if (!(interp->builtins.Array = arrayobject_createclass(interp))) goto error;
	if (!(interp->builtins.Integer = integerobject_createclass(interp))) goto error;
	if (!(interp->builtins.AstNode = astnodeobject_createclass(interp))) goto error;
	if (!(interp->builtins.Scope = scopeobject_createclass(interp))) goto error;
	if (!(interp->builtins.Block = blockobject_createclass(interp))) goto error;
	if (!(interp->builtins.StackFrame = stackframeobject_createclass(interp))) goto error;
	if (!(interp->builtins.MarkerError = errorobject_createmarkererrorclass(interp))) goto error;
	if (!(interp->builtins.ArbitraryAttribs = libraryobject_createaaclass(interp))) goto error;
	if (!(interp->builtins.Library = libraryobject_createclass(interp))) goto error;

	if (!(interp->builtinscope = scopeobject_newbuiltin(interp))) goto error;

	if (!(interp->importstuff.filelibcache = mappingobject_newempty(interp))) goto error;
	if (!(interp->importstuff.importers = arrayobject_newempty(interp))) goto error;
	if (!import_init(interp)) goto error;

	if (!(interp->oparrays.add = arrayobject_newempty(interp))) goto error;
	if (!(interp->oparrays.sub = arrayobject_newempty(interp))) goto error;
	if (!(interp->oparrays.mul = arrayobject_newempty(interp))) goto error;
	if (!(interp->oparrays.div = arrayobject_newempty(interp))) goto error;
	if (!(interp->oparrays.eq = arrayobject_newempty(interp))) goto error;
	if (!(interp->oparrays.lt = arrayobject_newempty(interp))) goto error;

	if (!stringobject_initoparrays(interp)) goto error;
	if (!integerobject_initoparrays(interp)) goto error;
	if (!mappingobject_initoparrays(interp)) goto error;

	if (!interpreter_addbuiltin(interp, "ArbitraryAttribs", interp->builtins.ArbitraryAttribs)) goto error;
	if (!interpreter_addbuiltin(interp, "Array", interp->builtins.Array)) goto error;
	if (!interpreter_addbuiltin(interp, "Block", interp->builtins.Block)) goto error;
	if (!interpreter_addbuiltin(interp, "Bool", interp->builtins.Bool)) goto error;
	if (!interpreter_addbuiltin(interp, "Error", interp->builtins.Error)) goto error;
	if (!interpreter_addbuiltin(interp, "Integer", interp->builtins.Integer)) goto error;
	if (!interpreter_addbuiltin(interp, "Mapping", interp->builtins.Mapping)) goto error;
	if (!interpreter_addbuiltin(interp, "MemError", interp->builtins.nomemerr->klass)) goto error;
	if (!interpreter_addbuiltin(interp, "Object", interp->builtins.Object)) goto error;
	if (!interpreter_addbuiltin(interp, "Option", interp->builtins.Option)) goto error;
	if (!interpreter_addbuiltin(interp, "Scope", interp->builtins.Scope)) goto error;
	if (!interpreter_addbuiltin(interp, "String", interp->builtins.String)) goto error;
	if (!interpreter_addbuiltin(interp, "true", interp->builtins.yes)) goto error;
	if (!interpreter_addbuiltin(interp, "false", interp->builtins.no)) goto error;
	if (!interpreter_addbuiltin(interp, "none", interp->builtins.none)) goto error;
	if (!interpreter_addbuiltin(interp, "null", interp->builtins.null)) goto error;
	if (!interpreter_addbuiltin(interp, "importers", interp->importstuff.importers)) goto error;

	if (!interpreter_addbuiltin(interp, "add_oparray", interp->oparrays.add)) goto error;
	if (!interpreter_addbuiltin(interp, "sub_oparray", interp->oparrays.sub)) goto error;
	if (!interpreter_addbuiltin(interp, "mul_oparray", interp->oparrays.mul)) goto error;
	if (!interpreter_addbuiltin(interp, "div_oparray", interp->oparrays.div)) goto error;
	if (!interpreter_addbuiltin(interp, "eq_oparray", interp->oparrays.eq)) goto error;
	if (!interpreter_addbuiltin(interp, "lt_oparray", interp->oparrays.lt)) goto error;

	if (!add_function(interp, "if", if_)) goto error;
	if (!add_function(interp, "throw", throw)) goto error;
	if (!add_function(interp, "lambda", lambdabuiltin)) goto error;
	if (!add_function(interp, "catch", catch)) goto error;
	if (!add_function(interp, "get_class", get_class)) goto error;
	if (!add_function(interp, "new", new)) goto error;
	if (!add_function(interp, "print", print)) goto error;
	if (!add_function(interp, "same_object", same_object)) goto error;
	if (!add_function(interp, "get_attrdata", get_attrdata)) goto error;
	if (!add_function(interp, "for", for_)) goto error;

	// compile like this:   $ CFLAGS=-DDEBUG_BUILTINS make clean all
#ifdef DEBUG_BUILTINS
	printf("things created by builtins_setup():\n");
#define debug(x) printf("  interp->%-20s = %p\n", #x, (void *) interp->x);
	debug(builtins.ArbitraryAttribs);
	debug(builtins.Array);
	debug(builtins.AstNode);
	debug(builtins.Block);
	debug(builtins.Bool);
	debug(builtins.Class);
	debug(builtins.Error);
	debug(builtins.Function);
	debug(builtins.Integer);
	debug(builtins.Library);
	debug(builtins.Mapping);
	debug(builtins.MarkerError);
	debug(builtins.Object);
	debug(builtins.Option);
	debug(builtins.Scope);
	debug(builtins.StackFrame);
	debug(builtins.String);
	debug(builtins.null);
	debug(builtins.yes);
	debug(builtins.no);
	debug(builtins.none);
	debug(builtins.nomemerr);

	debug(builtinscope);
	debug(err);
#undef debug
#endif   // DEBUG_BUILTINS

	assert(!(interp->err));
	return true;

error:
	assert(interp->err);
	struct Object *errsave = interp->err;
	interp->err = NULL;
	errorobject_print(interp, errsave);
	OBJECT_DECREF(interp, errsave);
	return false;

nomem:
	fprintf(stderr, "%s: not enough memory for setting up builtins\n", interp->argv0);
	return false;
}


void builtins_teardown(struct Interpreter *interp)
{
#define TEARDOWN(x) if (interp->x) { OBJECT_DECREF(interp, interp->x); interp->x = NULL; }
	TEARDOWN(builtins.ArbitraryAttribs);
	TEARDOWN(builtins.Array);
	TEARDOWN(builtins.AstNode);
	TEARDOWN(builtins.Block);
	TEARDOWN(builtins.Bool);
	TEARDOWN(builtins.Class);
	TEARDOWN(builtins.Error);
	TEARDOWN(builtins.Function);
	TEARDOWN(builtins.Integer);
	TEARDOWN(builtins.Library);
	TEARDOWN(builtins.Mapping);
	TEARDOWN(builtins.MarkerError);
	TEARDOWN(builtins.Object);
	TEARDOWN(builtins.Option);
	TEARDOWN(builtins.Scope);
	TEARDOWN(builtins.StackFrame);
	TEARDOWN(builtins.String);
	TEARDOWN(builtins.yes);
	TEARDOWN(builtins.no);
	TEARDOWN(builtins.none);
	TEARDOWN(builtins.null);
	TEARDOWN(builtins.nomemerr);
	TEARDOWN(builtinscope);
	TEARDOWN(importstuff.filelibcache);
	TEARDOWN(importstuff.importers);
	TEARDOWN(oparrays.add);
	TEARDOWN(oparrays.sub);
	TEARDOWN(oparrays.mul);
	TEARDOWN(oparrays.div);
	TEARDOWN(oparrays.eq);
	TEARDOWN(oparrays.lt);
	TEARDOWN(strings.else_);
	TEARDOWN(strings.empty);
	TEARDOWN(strings.export);
	TEARDOWN(strings.return_);
#undef TEARDOWN
}
