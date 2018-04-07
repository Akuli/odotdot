#include "run.h"
#include <assert.h>
#include <stddef.h>
#include <stdlib.h>
#include "ast.h"
#include "common.h"
#include "context.h"
#include "interpreter.h"
#include "objectsystem.h"
#include "objects/array.h"
#include "objects/function.h"
#include "objects/integer.h"
#include "objects/string.h"

// RETURNS A NEW REFERENCE
static struct Object *run_expression(struct Context *ctx, struct Object **errptr, struct AstNode *expr)
{
	switch(expr->kind) {
#define INFO_AS(X) ((struct X *)expr->info)
	case AST_GETVAR:
		return context_getvar(ctx, errptr, INFO_AS(AstGetVarInfo)->varname);

	case AST_STR:
		return stringobject_newfromustr(ctx->interp, errptr, *((struct UnicodeString *)expr->info));

	case AST_ARRAY:
		// this is handled specially because malloc(0) MAY return NULL
		if (INFO_AS(AstArrayOrBlockInfo)->nitems == 0)
			return arrayobject_newempty(ctx->interp, errptr);

		// TODO: create a DynamicArray directly instead of a temporary shit
		struct Object **elems = malloc(sizeof(struct Object *) * INFO_AS(AstArrayOrBlockInfo)->nitems);
		if (!elems) {
			*errptr = ctx->interp->nomemerr;
			return NULL;
		}

		for (size_t i=0; i < INFO_AS(AstArrayOrBlockInfo)->nitems; i++) {
			elems[i] = run_expression(ctx, errptr, INFO_AS(AstArrayOrBlockInfo)->items[i]);
			if (!elems[i]) {
				for (size_t j=0; j<i; j++)
					OBJECT_DECREF(ctx->interp, elems[i]);
				free(elems);
				return NULL;
			}
		}

		struct Object *arr = arrayobject_new(ctx->interp, errptr, elems, INFO_AS(AstArrayOrBlockInfo)->nitems);
		for (size_t i=0; i < INFO_AS(AstArrayOrBlockInfo)->nitems; i++)
			OBJECT_DECREF(ctx->interp, elems[i]);
		free(elems);
		return arr;

	case AST_INT:
		return integerobject_newfromcharptr(ctx->interp, errptr, INFO_AS(AstIntInfo)->valstr);
#undef INFO_AS

	default:
		assert(0);
	}
}

int run_statement(struct Context *ctx, struct Object **errptr, struct AstNode *stmt)
{
#define INFO_AS(X) ((struct X *)stmt->info)
	if (stmt->kind == AST_CALL) {
		struct Object *func = run_expression(ctx, errptr, INFO_AS(AstCallInfo)->func);
		if (!func)
			return STATUS_ERROR;

		// TODO: better type check
		assert(func->klass == ctx->interp->functionclass);

		// TODO: optimize with the at most 100 args limit?
		struct Object **args = malloc(sizeof(struct Object*) * INFO_AS(AstCallInfo)->nargs);
		if (!args) {
			OBJECT_DECREF(ctx->interp, func);
			*errptr = ctx->interp->nomemerr;
			return STATUS_ERROR;
		}

		for (size_t i=0; i < INFO_AS(AstCallInfo)->nargs; i++) {
			struct Object *arg = run_expression(ctx, errptr, INFO_AS(AstCallInfo)->args[i]);
			if (!arg) {
				for (size_t j=0; j<i; j++)
					OBJECT_DECREF(ctx->interp, args[j]);
				free(args);
				OBJECT_DECREF(ctx->interp, func);
				return STATUS_ERROR;
			}
			args[i] = arg;
		}

		struct Object *res = (functionobject_getcfunc(ctx->interp, func))(ctx, errptr, args, INFO_AS(AstCallInfo)->nargs);
		for (size_t i=0; i < INFO_AS(AstCallInfo)->nargs; i++)
			OBJECT_DECREF(ctx->interp, args[i]);
		free(args);
		OBJECT_DECREF(ctx->interp, func);

		if (res) {
			OBJECT_DECREF(ctx->interp, res);
			return STATUS_OK;
		} else {
			return STATUS_ERROR;
		}
	}

	if (stmt->kind == AST_CREATEVAR) {
		struct Object *val = run_expression(ctx, errptr, INFO_AS(AstCreateOrSetVarInfo)->val);
		if (!val)
			return STATUS_ERROR;

		int status = context_setlocalvar(ctx, errptr, INFO_AS(AstCreateOrSetVarInfo)->varname, val);
		OBJECT_DECREF(ctx->interp, val);
		return status;
	}

#undef INFO_AS
	assert(0);
}
