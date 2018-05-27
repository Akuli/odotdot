#include "block.h"
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


static struct Object *setup(struct Interpreter *interp, struct Object *argarr)
{
	if (check_args(interp, argarr, interp->builtins.blockclass, interp->builtins.scopeclass, interp->builtins.arrayclass, NULL) == STATUS_ERROR)
		return NULL;
	struct Object *block = ARRAYOBJECT_GET(argarr, 0);

	if (attribute_set(interp, block, "definition_scope", ARRAYOBJECT_GET(argarr, 1)) == STATUS_ERROR) return NULL;
	if (attribute_set(interp, block, "ast_statements", ARRAYOBJECT_GET(argarr, 2)) == STATUS_ERROR) return NULL;
	return interpreter_getbuiltin(interp, "null");
}

static struct Object *run_block(struct Interpreter *interp, struct Object *argarr)
{
	if (check_args(interp, argarr, interp->builtins.blockclass, interp->builtins.scopeclass, NULL) == STATUS_ERROR)
		return NULL;
	struct Object *block = ARRAYOBJECT_GET(argarr, 0);
	struct Object *scope = ARRAYOBJECT_GET(argarr, 1);

	struct Object *ast = attribute_get(interp, block, "ast_statements");
	if (!ast)
		return NULL;
	if (check_type(interp, interp->builtins.arrayclass, ast) == STATUS_ERROR)
		goto error;

	for (size_t i=0; i < ARRAYOBJECT_LEN(ast); i++) {
		if (check_type(interp, interp->builtins.astnodeclass, ARRAYOBJECT_GET(ast, i)) == STATUS_ERROR) goto error;
		if (run_statement(interp, scope, ARRAYOBJECT_GET(ast, i)) == STATUS_ERROR) goto error;
	}

	OBJECT_DECREF(interp, ast);
	return interpreter_getbuiltin(interp, "null");

error:
	OBJECT_DECREF(interp, ast);
	return NULL;
}

struct Object *blockobject_createclass(struct Interpreter *interp)
{
	struct Object *klass = classobject_new(interp, "Block", interp->builtins.objectclass, 1, NULL);
	if (!klass)
		return NULL;

	if (method_add(interp, klass, "setup", setup) == STATUS_ERROR) goto error;
	if (method_add(interp, klass, "run", run_block) == STATUS_ERROR) goto error;
	return klass;

error:
	OBJECT_DECREF(interp, klass);
	return NULL;
}

struct Object *blockobject_new(struct Interpreter *interp, struct Object *definitionscope, struct Object *astnodearr)
{
	struct Object *block = classobject_newinstance(interp, interp->builtins.blockclass, NULL, NULL);
	if (!block)
		return NULL;

	if (attribute_set(interp, block, "definition_scope", definitionscope) == STATUS_ERROR) goto error;
	if (attribute_set(interp, block, "ast_statements", astnodearr) == STATUS_ERROR) goto error;
	return block;

error:
	OBJECT_DECREF(interp, block);
	return NULL;
}
