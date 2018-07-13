#include "block.h"
#include <assert.h>
#include <stddef.h>
#include <stdlib.h>
#include "../attribute.h"
#include "../check.h"
#include "../interpreter.h"   // IWYU pragma: keep
#include "../method.h"
#include "../objectsystem.h"  // IWYU pragma: keep
#include "../runast.h"
#include "../stack.h"
#include "array.h"
#include "astnode.h"
#include "classobject.h"
#include "errors.h"
#include "function.h"
#include "mapping.h"
#include "scope.h"
#include "string.h"

static void blockdata_foreachref(void *data, object_foreachrefcb cb, void *cbdata)
{
	struct BlockObjectData *bod = data;
	cb(bod->definition_scope, cbdata);
	cb(bod->ast_statements, cbdata);
}

static void blockdata_destructor(void *data) {
	free(data);
}

static struct Object *newinstance(struct Interpreter *interp, struct Object *args, struct Object *opts)
{
	if (!check_args(interp, args, interp->builtins.Class, interp->builtins.Scope, interp->builtins.Array, NULL)) return NULL;
	if (!check_no_opts(interp, opts)) return NULL;

	struct BlockObjectData *data = malloc(sizeof *data);
	if (!data) {
		errorobject_thrownomem(interp);
		return NULL;
	}
	data->definition_scope = ARRAYOBJECT_GET(args, 1);
	data->ast_statements = ARRAYOBJECT_GET(args, 2);
	OBJECT_INCREF(interp, data->definition_scope);
	OBJECT_INCREF(interp, data->ast_statements);

	struct Object *block = object_new_noerr(interp, ARRAYOBJECT_GET(args, 0), (struct ObjectData){.data=data, .foreachref=blockdata_foreachref, .destructor=blockdata_destructor});
	if (!block) {
		errorobject_thrownomem(interp);
		OBJECT_DECREF(interp, data->definition_scope);
		OBJECT_DECREF(interp, data->ast_statements);
		return NULL;
	}
	return block;
}

// overrides Object's setup to allow arguments
static bool setup(struct Interpreter *interp, struct ObjectData nulldata, struct Object *args, struct Object *opts) {
	assert(0);   // TODO: test this setup and remove the assert... the setup doesn't seem to be ever actually called
	return true;
}

ATTRIBUTE_DEFINE_STRUCTDATA_GETTER(Block, BlockObjectData, definition_scope)
ATTRIBUTE_DEFINE_STRUCTDATA_GETTER(Block, BlockObjectData, ast_statements)


bool blockobject_run(struct Interpreter *interp, struct Object *block, struct Object *scope)
{
	for (size_t i=0; i < ARRAYOBJECT_LEN(BLOCKOBJECT_ASTSTMTS(block)); i++) {
		struct Object *node = ARRAYOBJECT_GET(BLOCKOBJECT_ASTSTMTS(block), i);

		// ast_statements is an array, so it's possible to add anything into it
		// must not have bad things happening, runast_statements expects AstNodes
		if (!check_type(interp, interp->builtins.AstNode, node))
			return false;

		// TODO: optimize by not pushing a new frame every time?
		struct AstNodeObjectData *astdata = node->objdata.data;
		if (!stack_push(interp, astdata->filename, astdata->lineno, scope))
			return false;
		bool ok = runast_statement(interp, scope, node);
		stack_pop(interp);
		if (!ok)
			return false;
	}
	return true;
}


void markerdata_foreachref(void *data, object_foreachrefcb cb, void *cbdata)
{
	cb((struct Object*)data, cbdata);
}

static bool returner_cfunc(struct Interpreter *interp, struct ObjectData markerdata, struct Object *args, struct Object *opts)
{

	if (!check_args(interp, args, interp->builtins.Object, NULL)) return false;
	if (!check_no_opts(interp, opts)) return false;

	struct Object *markererr = markerdata.data;
	if (!attribute_settoattrdata(interp, markererr, "value", ARRAYOBJECT_GET(args, 0)))
		return false;

	errorobject_throw(interp, markererr);
	return false;
}

static bool delete_returner(struct Interpreter *interp, struct Object *scope)
{
	struct Object *returner;
	int status = mappingobject_getanddelete(interp, SCOPEOBJECT_LOCALVARS(scope), interp->strings.return_, &returner);
	if (status == 0)    // someone deleted the returner... why not i guess
		return true;
	if (status == 1) {
		OBJECT_DECREF(interp, returner);
		return true;
	}

	assert(status == -1);   // error from mappingobject_get
	return false;
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

	struct Object *returner = functionobject_new(interp, (struct ObjectData){.data=marker, .foreachref=markerdata_foreachref, .destructor=NULL}, functionobject_mkcfunc_noret(returner_cfunc), "return");
	if (!returner) {
		OBJECT_DECREF(interp, marker);
		return NULL;
	}
	// add a new reference for the returner
	OBJECT_INCREF(interp, marker);

	bool ok = mappingobject_set(interp, SCOPEOBJECT_LOCALVARS(scope), interp->strings.return_, returner);
	OBJECT_DECREF(interp, returner);
	if (!ok) {
		OBJECT_DECREF(interp, marker);
		return NULL;
	}

	if (blockobject_run(interp, block, scope)) {
		// it didn't return
		// FIXME: ValueError feels wrong
		errorobject_throwfmt(interp, "ValueError", "return wasn't called");
		OBJECT_DECREF(interp, marker);
		return NULL;
	}
	assert(interp->err);

	if (interp->err == marker) {
		// the returner was called
		OBJECT_DECREF(interp, interp->err);
		interp->err = NULL;

		if (!delete_returner(interp, scope)) {
			OBJECT_DECREF(interp, marker);
			return NULL;
		}

		struct Object *retval = attribute_getfromattrdata(interp, marker, "value");
		OBJECT_DECREF(interp, marker);
		return retval;    // may be NULL or functionobject_noreturn
	}

	// it failed, but delete_returner() calls mappingobject_get()
	// for that, we need interp->err set to NULL temporarily
	struct Object *errsave = interp->err;
	interp->err = NULL;

	if (!delete_returner(interp, scope)) {
		// these cases are very rare, so discarding the original error is not bad imo
		OBJECT_DECREF(interp, errsave);
		OBJECT_DECREF(interp, marker);
		return NULL;
	}

	errorobject_throw(interp, errsave);
	OBJECT_DECREF(interp, errsave);
	OBJECT_DECREF(interp, marker);
	return NULL;
}


// TODO: a with_return option instead of two separate thingss
static bool run(struct Interpreter *interp, struct ObjectData thisdata, struct Object *args, struct Object *opts)
{
	if (!check_args(interp, args, interp->builtins.Scope, NULL)) return NULL;
	if (!check_no_opts(interp, opts)) return NULL;
	struct Object *block = thisdata.data;
	struct Object *scope = ARRAYOBJECT_GET(args, 0);
	return blockobject_run(interp, block, scope);
}

static struct Object *run_with_return(struct Interpreter *interp, struct ObjectData thisdata, struct Object *args, struct Object *opts)
{
	if (!check_args(interp, args, interp->builtins.Scope, NULL)) return NULL;
	if (!check_no_opts(interp, opts)) return NULL;
	struct Object *block = thisdata.data;
	struct Object *scope = ARRAYOBJECT_GET(args, 0);
	return blockobject_runwithreturn(interp, block, scope);
}

struct Object *blockobject_createclass(struct Interpreter *interp)
{
	struct Object *klass = classobject_new(interp, "Block", interp->builtins.Object, newinstance);
	if (!klass)
		return NULL;

	if (!method_add_noret(interp, klass, "setup", setup)) goto error;
	if (!method_add_noret(interp, klass, "run", run)) goto error;
	if (!method_add_yesret(interp, klass, "run_with_return", run_with_return)) goto error;
	if (!attribute_add(interp, klass, "definition_scope", definition_scope_getter, NULL)) goto error;
	if (!attribute_add(interp, klass, "ast_statements", ast_statements_getter, NULL)) goto error;
	return klass;

error:
	OBJECT_DECREF(interp, klass);
	return NULL;
}

struct Object *blockobject_new(struct Interpreter *interp, struct Object *definition_scope, struct Object *astnodearr)
{
	struct BlockObjectData *data = malloc(sizeof *data);
	if (!data) {
		errorobject_thrownomem(interp);
		return NULL;
	}
	data->definition_scope = definition_scope;
	data->ast_statements = astnodearr;
	OBJECT_INCREF(interp, definition_scope);
	OBJECT_INCREF(interp, astnodearr);

	struct Object *block = object_new_noerr(interp, interp->builtins.Block, (struct ObjectData){.data=data, .foreachref=blockdata_foreachref, .destructor=blockdata_destructor});
	if (!block) {
		errorobject_thrownomem(interp);
		OBJECT_DECREF(interp, definition_scope);
		OBJECT_DECREF(interp, astnodearr);
		return NULL;
	}
	return block;
}
