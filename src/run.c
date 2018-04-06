#include "run.h"
#include <assert.h>
#include <stddef.h>
#include <stdlib.h>
#include "ast.h"
#include "common.h"
#include "context.h"
#include "interpreter.h"
#include "objectsystem.h"
#include "objects/function.h"
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
#undef INFO_AS
	default:
		assert(0);
	}
}

int run_statement(struct Context *ctx, struct Object **errptr, struct AstNode *stmt)
{
	switch(stmt->kind) {
#define INFO_AS(X) ((struct X *)stmt->info)
	case AST_CALL:
		(void) INFO_AS(AstCallInfo)->func;
		struct Object *func = run_expression(ctx, errptr, INFO_AS(AstCallInfo)->func);
		if (!func)
			return STATUS_ERROR;

		// TODO: better type check
		assert(func->klass == ctx->interp->functionobjectinfo);

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
#undef INFO_AS
	default:
		assert(0);
	}
}
