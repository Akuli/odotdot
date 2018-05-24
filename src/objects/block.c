#include "block.h"
#include "../attribute.h"
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


static struct Object *run_block(struct Interpreter *interp, struct Object **args, size_t nargs)
{
	if (functionobject_checktypes(interp, args, nargs, interp->builtins.blockclass, interp->builtins.scopeclass, NULL) == STATUS_ERROR)
		return NULL;
	struct Object *block = args[0];
	struct Object *scope = args[1];

	struct Object *ast = attribute_get(interp, block, "ast_statements");
	if (!ast)
		return NULL;
	if (errorobject_typecheck(interp, interp->builtins.arrayclass, ast) == STATUS_ERROR) {
		OBJECT_DECREF(interp, ast);
		return NULL;
	}

	for (size_t i=0; i < ARRAYOBJECT_LEN(ast); i++) {
		if (errorobject_typecheck(interp, interp->builtins.astnodeclass, ARRAYOBJECT_GET(ast, i)) == STATUS_ERROR) {
			OBJECT_DECREF(interp, ast);
			return NULL;
		}
		if (run_statement(interp, scope, ARRAYOBJECT_GET(ast, i)) == STATUS_ERROR) {
			OBJECT_DECREF(interp, ast);
			return NULL;
		}
	}

	OBJECT_DECREF(interp, ast);
	return stringobject_newfromcharptr(interp, "asd");    // TODO: we need a null object :^/
}

struct Object *blockobject_createclass(struct Interpreter *interp)
{
	struct Object *klass = classobject_new(interp, "Block", interp->builtins.objectclass, 1, NULL);
	if (!klass)
		return NULL;

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
