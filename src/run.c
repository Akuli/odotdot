#include "run.h"
#include <assert.h>
#include <stddef.h>
#include <stdlib.h>
#include "attribute.h"
#include "check.h"
#include "common.h"
#include "interpreter.h"
#include "method.h"
#include "objectsystem.h"
#include "objects/array.h"
#include "objects/astnode.h"
#include "objects/block.h"
#include "objects/errors.h"
#include "objects/function.h"
#include "objects/mapping.h"
#include "objects/scope.h"
#include "objects/string.h"
#include "parse.h"

// RETURNS A NEW REFERENCE
static struct Object *run_expression(struct Interpreter *interp, struct Object *scope, struct Object *exprnode)
{
	struct AstNodeObjectData *nodedata = exprnode->data;

#define INFO_AS(X) ((struct X *) nodedata->info)
	if (nodedata->kind == AST_GETVAR) {
		struct Object *name = stringobject_newfromustr(interp, INFO_AS(AstGetVarInfo)->varname);
		if (!name)
			return NULL;

		// TODO: expose scope get_var in C to avoid method_call?
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
		else {
			struct Object *s = stringobject_newfromustr(interp, INFO_AS(AstGetAttrOrMethodInfo)->name);
			if (!s)
				return NULL;
			res = attribute_getwithstringobj(interp, obj, s);
			OBJECT_DECREF(interp, s);
		}
		OBJECT_DECREF(interp, obj);
		return res;
	}

	if (nodedata->kind == AST_CALL) {
		struct Object *func = run_expression(interp, scope, INFO_AS(AstCallInfo)->funcnode);
		if (!func)
			return NULL;
		if (check_type(interp, interp->builtins.functionclass, func) == STATUS_ERROR) {
			OBJECT_DECREF(interp, func);
			return NULL;
		}

		struct Object *argarr = arrayobject_newempty(interp);
		if (!argarr) {
			OBJECT_DECREF(interp, func);
			return NULL;
		}
		for (size_t i=0; i < INFO_AS(AstCallInfo)->nargs; i++) {
			struct Object *arg = run_expression(interp, scope, INFO_AS(AstCallInfo)->argnodes[i]);
			if (!arg) {
				OBJECT_DECREF(interp, argarr);
				OBJECT_DECREF(interp, func);
				return NULL;
			}
			int ret = arrayobject_push(interp, argarr, arg);
			OBJECT_DECREF(interp, arg);
			if (ret == STATUS_ERROR) {
				OBJECT_DECREF(interp, argarr);
				OBJECT_DECREF(interp, func);
				return NULL;
			}
		}

		struct Object *res = functionobject_vcall(interp, func, argarr);
		OBJECT_DECREF(interp, argarr);
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
	struct AstNodeObjectData *nodedata = stmtnode->data;

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

		int res = mappingobject_set(interp, localvars, keystr, val);
		OBJECT_DECREF(interp, keystr);
		OBJECT_DECREF(interp, localvars);
		OBJECT_DECREF(interp, val);
		return res;
	}

	if (nodedata->kind == AST_SETVAR) {
		struct Object *namestr = stringobject_newfromustr(interp, INFO_AS(AstCreateOrSetVarInfo)->varname);
		if (!namestr)
			return STATUS_ERROR;

		struct Object *val = run_expression(interp, scope, INFO_AS(AstCreateOrSetVarInfo)->valnode);
		if (!val) {
			OBJECT_DECREF(interp, namestr);
			return STATUS_ERROR;
		}

		// TODO: expose Scope::set_var?
		struct Object *res = method_call(interp, scope, "set_var", namestr, val, NULL);
		OBJECT_DECREF(interp, namestr);
		OBJECT_DECREF(interp, val);

		if (res) {
			OBJECT_DECREF(interp, res);
			return STATUS_OK;
		}
		return STATUS_ERROR;
	}

	if (nodedata->kind == AST_SETATTR) {
		struct Object *obj = run_expression(interp, scope, INFO_AS(AstSetAttrInfo)->objnode);
		if (!obj)
			return STATUS_ERROR;

		struct Object *val = run_expression(interp, scope, INFO_AS(AstSetAttrInfo)->valnode);
		if (!val) {
			OBJECT_DECREF(interp, obj);
			return STATUS_ERROR;
		}

		struct Object *stringobj = stringobject_newfromustr(interp, INFO_AS(AstSetAttrInfo)->attr);
		if (!stringobj) {
			OBJECT_DECREF(interp, val);
			OBJECT_DECREF(interp, obj);
			return STATUS_ERROR;
		}

		int res = attribute_setwithstringobj(interp, obj, stringobj, val);
		OBJECT_DECREF(interp, obj);
		OBJECT_DECREF(interp, val);
		OBJECT_DECREF(interp, stringobj);
		return res;
	}
#undef INFO_AS

	assert(0);
}
