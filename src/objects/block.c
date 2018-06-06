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
#include "null.h"
#include "string.h"

ATTRIBUTE_DEFINE_SIMPLE_GETTER(definition_scope, Block)
ATTRIBUTE_DEFINE_SIMPLE_GETTER(ast_statements, Block)

static struct Object *setup(struct Interpreter *interp, struct Object *argarr)
{
	if (check_args(interp, argarr, interp->builtins.Block, interp->builtins.Scope, interp->builtins.Array, NULL) == STATUS_ERROR)
		return NULL;

	struct Object *block = ARRAYOBJECT_GET(argarr, 0);
	if (attribute_settoattrdata(interp, block, "definition_scope", ARRAYOBJECT_GET(argarr, 1)) == STATUS_ERROR) return NULL;
	if (attribute_settoattrdata(interp, block, "ast_statements", ARRAYOBJECT_GET(argarr, 2)) == STATUS_ERROR) return NULL;
	return nullobject_get(interp);
}

int blockobject_run(struct Interpreter *interp, struct Object *block, struct Object *scope)
{
	struct Object *ast = attribute_get(interp, block, "ast_statements");
	if (!ast)
		return STATUS_ERROR;
	if (check_type(interp, interp->builtins.Array, ast) == STATUS_ERROR)
		goto error;

	for (size_t i=0; i < ARRAYOBJECT_LEN(ast); i++) {
		// ast_statements attribute is an array, so it's possible to add anything into it
		// must not have bad things happening, run_statements expects AstNodes
		if (check_type(interp, interp->builtins.AstNode, ARRAYOBJECT_GET(ast, i)) == STATUS_ERROR) goto error;
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
	if (check_args(interp, argarr, interp->builtins.Block, interp->builtins.Scope, NULL) == STATUS_ERROR)
		return NULL;
	return (blockobject_run(interp, ARRAYOBJECT_GET(argarr, 0), ARRAYOBJECT_GET(argarr, 1)) == STATUS_OK)
		? nullobject_get(interp) : NULL;
}

struct Object *blockobject_createclass(struct Interpreter *interp)
{
	struct Object *klass = classobject_new(interp, "Block", interp->builtins.Object, NULL);
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
	struct Object *block = classobject_newinstance(interp, interp->builtins.Block, NULL, NULL);
	if (!block)
		return NULL;

	if (attribute_settoattrdata(interp, block, "definition_scope", definition_scope) == STATUS_ERROR) goto error;
	if (attribute_settoattrdata(interp, block, "ast_statements", astnodearr) == STATUS_ERROR) goto error;
	return block;

error:
	OBJECT_INCREF(interp, block);
	return NULL;
}
