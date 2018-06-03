#include "block.h"
#include <stdlib.h>
#include "../attribute.h"
#include "../check.h"
#include "../common.h"
#include "../interpreter.h"   // IWYU pragma: keep
#include "../method.h"
#include "../objectsystem.h"  // IWYU pragma: keep
#include "../run.h"
#include "array.h"
#include "classobject.h"
#include "errors.h"
#include "function.h"
#include "string.h"

struct BlockData {
	struct Object *definition_scope;
	struct Object *ast_statements;
};

// boilerplates
// TODO: should definition_scope or ast_statements be settable? MUHAHAHAAA
static struct Object *definition_scope_getter(struct Interpreter *interp, struct Object *argarr)
{
	if (check_args(interp, argarr, interp->builtins.blockclass, NULL) == STATUS_ERROR)
		return NULL;
	struct Object *res = ((struct BlockData *) ARRAYOBJECT_GET(argarr, 0)->data)->definition_scope;
	OBJECT_INCREF(interp, res);
	return res;
}
static struct Object *ast_statements_getter(struct Interpreter *interp, struct Object *argarr)
{
	if (check_args(interp, argarr, interp->builtins.blockclass, NULL) == STATUS_ERROR)
		return NULL;
	struct Object *res = ((struct BlockData *) ARRAYOBJECT_GET(argarr, 0)->data)->ast_statements;
	OBJECT_INCREF(interp, res);
	return res;
}
static void foreachref(struct Object *obj, void *cbdata, classobject_foreachrefcb cb)
{
	if (obj->data) {
		struct BlockData *data = obj->data;
		cb(data->definition_scope, cbdata);
		cb(data->ast_statements, cbdata);
	}
}
static void destructor(struct Object *obj)
{
	if (obj->data)
		free(obj->data);
}


static struct Object *setup(struct Interpreter *interp, struct Object *argarr)
{
	if (check_args(interp, argarr, interp->builtins.blockclass, interp->builtins.scopeclass, interp->builtins.arrayclass, NULL) == STATUS_ERROR)
		return NULL;
	struct Object *block = ARRAYOBJECT_GET(argarr, 0);
	struct Object *definition_scope = ARRAYOBJECT_GET(argarr, 1);
	struct Object *ast_statements = ARRAYOBJECT_GET(argarr, 2);

	if (block->data) {
		errorobject_setwithfmt(interp, "setup was called twice");
		return NULL;
	}

	struct BlockData *data = malloc(sizeof(struct BlockData));
	if (!data) {
		errorobject_setnomem(interp);
		return NULL;
	}

	data->definition_scope = definition_scope;
	data->ast_statements = ast_statements;
	OBJECT_INCREF(interp, definition_scope);
	OBJECT_INCREF(interp, ast_statements);
	return interpreter_getbuiltin(interp, "null");
}

int blockobject_run(struct Interpreter *interp, struct Object *block, struct Object *scope)
{
	struct Object *ast = attribute_get(interp, block, "ast_statements");
	if (!ast)
		return STATUS_ERROR;
	if (check_type(interp, interp->builtins.arrayclass, ast) == STATUS_ERROR)
		goto error;

	for (size_t i=0; i < ARRAYOBJECT_LEN(ast); i++) {
		// ast_statements attribute is an array, so it's possible to add anything into it
		// must not have bad things happening, run_statements expects AstNodes
		if (check_type(interp, interp->builtins.astnodeclass, ARRAYOBJECT_GET(ast, i)) == STATUS_ERROR) goto error;
		if (run_statement(interp, scope, ARRAYOBJECT_GET(ast, i)) == STATUS_ERROR) goto error;
	}

	OBJECT_DECREF(interp, ast);
	return STATUS_OK;

error:
	OBJECT_DECREF(interp, ast);
	return STATUS_ERROR;
}

static struct Object *run(struct Interpreter *interp, struct Object *argarr)
{
	if (check_args(interp, argarr, interp->builtins.blockclass, interp->builtins.scopeclass, NULL) == STATUS_ERROR)
		return NULL;
	return (blockobject_run(interp, ARRAYOBJECT_GET(argarr, 0), ARRAYOBJECT_GET(argarr, 1)) == STATUS_OK)
		? interpreter_getbuiltin(interp, "null") : NULL;
}

struct Object *blockobject_createclass(struct Interpreter *interp)
{
	struct Object *klass = classobject_new(interp, "Block", interp->builtins.objectclass, foreachref);
	if (!klass)
		return NULL;

	if (method_add(interp, klass, "setup", setup) == STATUS_ERROR) goto error;
	if (method_add(interp, klass, "run", run) == STATUS_ERROR) goto error;
	if (attribute_add(interp, klass, "definition_scope", definition_scope_getter, NULL) == STATUS_ERROR) goto error;
	if (attribute_add(interp, klass, "ast_statements", ast_statements_getter, NULL) == STATUS_ERROR) goto error;
	return klass;

error:
	OBJECT_DECREF(interp, klass);
	return NULL;
}

struct Object *blockobject_new(struct Interpreter *interp, struct Object *definition_scope, struct Object *astnodearr)
{
	struct BlockData *data = malloc(sizeof(struct BlockData));
	if (!data) {
		errorobject_setnomem(interp);
		return NULL;
	}
	data->definition_scope = definition_scope;
	data->ast_statements = astnodearr;
	OBJECT_INCREF(interp, definition_scope);
	OBJECT_INCREF(interp, astnodearr);

	struct Object *block = classobject_newinstance(interp, interp->builtins.blockclass, data, destructor);
	if (!block) {
		OBJECT_DECREF(interp, definition_scope);
		OBJECT_DECREF(interp, astnodearr);
		free(data);
		return NULL;
	}

	return block;
}
