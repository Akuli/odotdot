#include "block.h"
#include <assert.h>
#include <stdlib.h>
#include "../attribute.h"
#include "../check.h"
#include "../interpreter.h"   // IWYU pragma: keep
#include "../method.h"
#include "../objectsystem.h"  // IWYU pragma: keep
#include "../run.h"
#include "array.h"
#include "astnode.h"
#include "classobject.h"
#include "errors.h"
#include "function.h"
#include "mapping.h"
#include "null.h"
#include "scope.h"
#include "string.h"

ATTRIBUTE_DEFINE_SIMPLE_GETTER(definition_scope, Block)
ATTRIBUTE_DEFINE_SIMPLE_GETTER(ast_statements, Block)

static struct Object *setup(struct Interpreter *interp, struct Object *args, struct Object *opts)
{
	if (!check_args(interp, args, interp->builtins.Block, interp->builtins.Scope, interp->builtins.Array, NULL)) return NULL;
	if (!check_no_opts(interp, opts)) return NULL;

	struct Object *block = ARRAYOBJECT_GET(args, 0);
	if (!attribute_settoattrdata(interp, block, "definition_scope", ARRAYOBJECT_GET(args, 1))) return NULL;
	if (!attribute_settoattrdata(interp, block, "ast_statements", ARRAYOBJECT_GET(args, 2))) return NULL;
	return nullobject_get(interp);
}


bool blockobject_run(struct Interpreter *interp, struct Object *block, struct Object *scope)
{
	struct Object *ast = attribute_get(interp, block, "ast_statements");
	if (!ast)
		return false;
	if (!check_type(interp, interp->builtins.Array, ast))
		goto error;

	for (size_t i=0; i < ARRAYOBJECT_LEN(ast); i++) {
		// ast_statements attribute is an array, so it's possible to add anything into it
		// must not have bad things happening, run_statements expects AstNodes
		if (!check_type(interp, interp->builtins.AstNode, ARRAYOBJECT_GET(ast, i)))
			goto error;

		// TODO: optimize by not pushing a new frame every time?
		struct AstNodeObjectData *astdata = ARRAYOBJECT_GET(ast, i)->data;
		if (!stack_push(interp, astdata->filename, astdata->lineno, scope))
			goto error;
		bool ok = run_statement(interp, scope, ARRAYOBJECT_GET(ast, i));
		stack_pop(interp);
		if (!ok)
			goto error;
	}

	OBJECT_DECREF(interp, ast);
	return true;

error:
	OBJECT_DECREF(interp, ast);
	return false;
}


// this is partialled to an error object that this thing must throw
static struct Object *returner_cfunc(struct Interpreter *interp, struct Object *args, struct Object *opts)
{
	assert(ARRAYOBJECT_LEN(args) >= 1 && classobject_isinstanceof(ARRAYOBJECT_GET(args, 0), interp->builtins.MarkerError));
	struct Object *err = ARRAYOBJECT_GET(args, 0);
	check_no_opts(interp, opts);

	struct Object *gonnareturn;
	if (ARRAYOBJECT_LEN(args) == 1)
		gonnareturn = interp->builtins.null;
	else if (ARRAYOBJECT_LEN(args) == 2)
		gonnareturn = ARRAYOBJECT_GET(args, 1);
	else {
		errorobject_throwfmt(interp, "ArgError", "return must be called with no args or exactly 1 arg");
		return NULL;
	}

	if (!attribute_settoattrdata(interp, err, "value", gonnareturn))
		return NULL;

	errorobject_throw(interp, err);
	return NULL;
}

struct Object *blockobject_runwithreturn(struct Interpreter *interp, struct Object *block, struct Object *scope)
{
	struct Object *markermsg = stringobject_newfromcharptr(interp, "if you see this error, something is wrong");
	if (!markermsg)
		return NULL;

	struct Object *marker = errorobject_new(interp, interp->builtins.MarkerError, markermsg);
	OBJECT_DECREF(interp, markermsg);
	if (!marker)
		return NULL;

	// TODO: optimize by not creating this every time?
	struct Object *rawreturner = functionobject_new(interp, returner_cfunc, "return");
	if (!rawreturner) {
		OBJECT_DECREF(interp, marker);
		return NULL;
	}

	struct Object *returner = functionobject_newpartial(interp, rawreturner, marker);
	OBJECT_DECREF(interp, rawreturner);
	if (!returner) {
		OBJECT_DECREF(interp, marker);
		return NULL;
	}

	// TODO: optimize by not creating this every time?
	struct Object *returnstr = stringobject_newfromcharptr(interp, "return");
	if (!returnstr) {
		OBJECT_DECREF(interp, returner);
		OBJECT_DECREF(interp, marker);
		return NULL;
	}

	bool ok = mappingobject_set(interp, SCOPEOBJECT_LOCALVARS(scope), returnstr, returner);
	OBJECT_DECREF(interp, returnstr);
	OBJECT_DECREF(interp, returner);
	if (!ok) {
		OBJECT_DECREF(interp, marker);
		return NULL;
	}

	// TODO: delete the returner from the scope after running

	if (blockobject_run(interp, block, scope)) {
		// it didn't return
		OBJECT_DECREF(interp, marker);
		return nullobject_get(interp);
	}
	assert(interp->err);

	if (interp->err == marker) {
		// the returner was called
		OBJECT_DECREF(interp, interp->err);
		interp->err = NULL;

		struct Object *retval = attribute_getfromattrdata(interp, marker, "value");
		OBJECT_DECREF(interp, marker);
		return retval;    // may be NULL
	}

	// it failed
	OBJECT_DECREF(interp, marker);
	return NULL;
}


// TODO: a with_return option instead of two separate thingss
static struct Object *run(struct Interpreter *interp, struct Object *args, struct Object *opts)
{
	if (!check_args(interp, args, interp->builtins.Block, interp->builtins.Scope, NULL)) return NULL;
	if (!check_no_opts(interp, opts)) return NULL;
	return blockobject_run(interp, ARRAYOBJECT_GET(args, 0), ARRAYOBJECT_GET(args, 1)) ? nullobject_get(interp) : NULL;
}

static struct Object *run_with_return(struct Interpreter *interp, struct Object *args, struct Object *opts)
{
	if (!check_args(interp, args, interp->builtins.Block, interp->builtins.Scope, NULL)) return NULL;
	if (!check_no_opts(interp, opts)) return NULL;
	return blockobject_runwithreturn(interp, ARRAYOBJECT_GET(args, 0), ARRAYOBJECT_GET(args, 1));
}

struct Object *blockobject_createclass(struct Interpreter *interp)
{
	struct Object *klass = classobject_new(interp, "Block", interp->builtins.Object, false);
	if (!klass)
		return NULL;

	if (!method_add(interp, klass, "setup", setup)) goto error;
	if (!method_add(interp, klass, "run", run)) goto error;
	if (!method_add(interp, klass, "run_with_return", run_with_return)) goto error;
	if (!attribute_add(interp, klass, "definition_scope", definition_scope_getter, NULL)) goto error;
	if (!attribute_add(interp, klass, "ast_statements", ast_statements_getter, NULL)) goto error;
	return klass;

error:
	OBJECT_DECREF(interp, klass);
	return NULL;
}

struct Object *blockobject_new(struct Interpreter *interp, struct Object *definition_scope, struct Object *astnodearr)
{
	struct Object *block = classobject_newinstance(interp, interp->builtins.Block, NULL, NULL, NULL);
	if (!block)
		return NULL;

	if (!attribute_settoattrdata(interp, block, "definition_scope", definition_scope)) goto error;
	if (!attribute_settoattrdata(interp, block, "ast_statements", astnodearr)) goto error;
	return block;

error:
	OBJECT_INCREF(interp, block);
	return NULL;
}
