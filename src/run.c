#include "run.h"
#include <assert.h>
#include <stddef.h>
#include <stdlib.h>
#include "ast.h"
#include "attribute.h"
#include "common.h"
#include "interpreter.h"
#include "method.h"
#include "objectsystem.h"
#include "objects/array.h"
#include "objects/block.h"
#include "objects/errors.h"
#include "objects/function.h"
#include "objects/scope.h"
#include "objects/string.h"

// RETURNS A NEW REFERENCE
static struct Object *run_expression(struct Interpreter *interp, struct Object *scope, struct Object *exprnode)
{
	struct AstNodeData *nodedata = exprnode->data;

#define INFO_AS(X) ((struct X *) nodedata->info)
	if (nodedata->kind == AST_GETVAR) {
		// TODO: optimize this :^/(
		struct Object *name = stringobject_newfromustr(interp, INFO_AS(AstGetVarInfo)->varname);
		if (!name)
			return NULL;
		struct Object *res = method_call(interp, scope, "get_var", name, NULL);
		OBJECT_DECREF(interp, name);
		return res;
	}

	if (nodedata->kind == AST_STR || nodedata->kind == AST_INT) {
		OBJECT_INCREF(interp, (struct Object *) nodedata->info);
		return nodedata->info;
	}

	if (nodedata->kind == AST_GETMETHOD || nodedata->kind == AST_GETATTR) {
		struct Object *obj = run_expression(interp, scope, INFO_AS(AstGetAttrOrMethodInfo)->objnode);
		if (!obj)
			return NULL;

		struct Object *res;
		if (nodedata->kind == AST_GETMETHOD)
			res = method_getwithustr(interp, obj, INFO_AS(AstGetAttrOrMethodInfo)->name);
		else
			res = attribute_getwithustr(interp, obj, INFO_AS(AstGetAttrOrMethodInfo)->name);
		OBJECT_DECREF(interp, obj);
		return res;
	}

	if (nodedata->kind == AST_CALL) {
		struct Object *func = run_expression(interp, scope, INFO_AS(AstCallInfo)->funcnode);
		if (!func)
			return NULL;

		// TODO: better error reporting
		assert(func->klass == interp->builtins.functionclass);

		struct Object **args = malloc(sizeof(struct Object*) * INFO_AS(AstCallInfo)->nargs);
		if (!args) {
			OBJECT_DECREF(interp, func);
			errorobject_setnomem(interp);
			return NULL;
		}

		for (size_t i=0; i < INFO_AS(AstCallInfo)->nargs; i++) {
			struct Object *arg = run_expression(interp, scope, INFO_AS(AstCallInfo)->argnodes[i]);
			if (!arg) {
				for (size_t j=0; j<i; j++)
					OBJECT_DECREF(interp, args[j]);
				free(args);
				OBJECT_DECREF(interp, func);
				return NULL;
			}
			args[i] = arg;
		}

		struct Object *res = functionobject_vcall(interp, func, args, INFO_AS(AstCallInfo)->nargs);
		for (size_t i=0; i < INFO_AS(AstCallInfo)->nargs; i++)
			OBJECT_DECREF(interp, args[i]);
		free(args);
		OBJECT_DECREF(interp, func);
		return res;
	}

	if (nodedata->kind == AST_ARRAY) {
		struct Object *result = arrayobject_newempty(interp);
		if (!result)
			return NULL;

		for (size_t i=0; i < ARRAYOBJECT_LEN(INFO_AS(AstArrayOrBlockInfo)); i++) {
			struct Object *elem = run_expression(interp, scope, ARRAYOBJECT_GET(INFO_AS(AstArrayOrBlockInfo), i));
			if (!elem) {
				OBJECT_DECREF(interp, result);
				return NULL;
			}
			int pushret = arrayobject_push(interp, result, elem);
			OBJECT_DECREF(interp, elem);
			if (pushret == STATUS_ERROR) {
				OBJECT_DECREF(interp, result);
				return NULL;
			}
		}
		return result;
	}

	if (nodedata->kind == AST_BLOCK)
		return blockobject_new(interp, scope, INFO_AS(AstArrayOrBlockInfo));
#undef INFO_AS

	assert(0);
}

int run_statement(struct Interpreter *interp, struct Object *scope, struct Object *stmtnode)
{
	struct AstNodeData *nodedata = stmtnode->data;

#define INFO_AS(X) ((struct X *) nodedata->info)
	if (nodedata->kind == AST_CALL) {
		struct Object *ret = run_expression(interp, scope, stmtnode);
		if (!ret)
			return STATUS_ERROR;

		OBJECT_DECREF(interp, ret);  // ignore the return value
		return STATUS_OK;
	}

	if (nodedata->kind == AST_CREATEVAR) {
		// TODO: optimize ://(///((((
		struct Object *val = run_expression(interp, scope, INFO_AS(AstCreateOrSetVarInfo)->valnode);
		if (!val)
			return STATUS_ERROR;

		struct Object *localvars = attribute_get(interp, scope, "local_vars");
		if (!localvars) {
			OBJECT_DECREF(interp, val);
			return STATUS_ERROR;
		}

		struct Object *keystr = stringobject_newfromustr(interp, INFO_AS(AstCreateOrSetVarInfo)->varname);
		if (!keystr) {
			OBJECT_DECREF(interp, localvars);
			OBJECT_DECREF(interp, val);
		}

		struct Object *ret = method_call(interp, localvars, "set", keystr, val, NULL);
		OBJECT_DECREF(interp, keystr);
		OBJECT_DECREF(interp, localvars);
		OBJECT_DECREF(interp, val);
		if (ret) {
			OBJECT_DECREF(interp, ret);
			return STATUS_OK;
		}
		return STATUS_ERROR;
	}
#undef INFO_AS

	assert(0);
}
