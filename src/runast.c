#include "runast.h"
#include <assert.h>
#include <stdbool.h>
#include <stddef.h>
#include "attribute.h"
#include "check.h"
#include "interpreter.h"
#include "objectsystem.h"
#include "objects/array.h"
#include "objects/astnode.h"
#include "objects/block.h"
#include "objects/function.h"
#include "objects/mapping.h"
#include "objects/scope.h"
#include "operator.h"

// TODO: this creates lots of new strings, avoid that to speed up the interpreter

// RETURNS A NEW REFERENCE
static struct Object *runast_expression(struct Interpreter *interp, struct Object *scope, struct Object *exprnode)
{
	struct AstNodeObjectData *nodedata = exprnode->objdata.data;

#define INFO_AS(X) ((struct X *) nodedata->info)
	if (nodedata->kind == AST_GETVAR)
		return scopeobject_getvar(interp, scope, INFO_AS(AstGetVarInfo)->varname);

	if (nodedata->kind == AST_STR || nodedata->kind == AST_INT) {
		OBJECT_INCREF(interp, (struct Object *) nodedata->info);
		return nodedata->info;
	}

	if (nodedata->kind == AST_GETATTR) {
		struct Object *obj = runast_expression(interp, scope, INFO_AS(AstGetAttrInfo)->objnode);
		if (!obj)
			return NULL;

		struct Object *res = attribute_getwithstringobj(interp, obj, INFO_AS(AstGetAttrInfo)->name);
		OBJECT_DECREF(interp, obj);
		return res;

	}

	if (nodedata->kind == AST_CALL) {
		// TODO: optimize this
		struct Object *func = runast_expression(interp, scope, INFO_AS(AstCallInfo)->funcnode);
		if (!func)
			return NULL;
		if (!check_type(interp, interp->builtins.Function, func)) {
			OBJECT_DECREF(interp, func);
			return NULL;
		}

		struct Object *args = arrayobject_newwithcapacity(interp, ARRAYOBJECT_LEN(INFO_AS(AstCallInfo)->args));
		if (!args) {
			OBJECT_DECREF(interp, func);
			return NULL;
		}
		for (size_t i=0; i < ARRAYOBJECT_LEN(INFO_AS(AstCallInfo)->args); i++) {
			struct Object *arg = runast_expression(interp, scope, ARRAYOBJECT_GET(INFO_AS(AstCallInfo)->args, i));
			if (!arg) {
				OBJECT_DECREF(interp, args);
				OBJECT_DECREF(interp, func);
				return NULL;
			}
			bool ret = arrayobject_push(interp, args, arg);
			OBJECT_DECREF(interp, arg);
			if (!ret) {
				OBJECT_DECREF(interp, args);
				OBJECT_DECREF(interp, func);
				return NULL;
			}
		}

		struct Object *opts = mappingobject_newempty(interp);
		if (!opts) {
			OBJECT_DECREF(interp, args);
			OBJECT_DECREF(interp, func);
			return NULL;
		}

		struct MappingObjectIter iter;
		mappingobject_iterbegin(&iter, INFO_AS(AstCallInfo)->opts);
		while (mappingobject_iternext(&iter)) {
			struct Object *val = runast_expression(interp, scope, iter.value);
			if (!val) {
				OBJECT_DECREF(interp, opts);
				OBJECT_DECREF(interp, args);
				OBJECT_DECREF(interp, func);
				return NULL;
			}

			bool ok = mappingobject_set(interp, opts, iter.key, val);
			OBJECT_DECREF(interp, val);
			if (!ok) {
				OBJECT_DECREF(interp, opts);
				OBJECT_DECREF(interp, args);
				OBJECT_DECREF(interp, func);
				return NULL;
			}
		}

		struct Object *res = functionobject_vcall(interp, func, args, opts);
		OBJECT_DECREF(interp, opts);
		OBJECT_DECREF(interp, args);
		OBJECT_DECREF(interp, func);
		return res;
	}

	if (nodedata->kind == AST_OPCALL) {
		struct Object *lhs = runast_expression(interp, scope, INFO_AS(AstOpCallInfo)->lhs);
		if (!lhs)
			return NULL;
		struct Object *rhs = runast_expression(interp, scope, INFO_AS(AstOpCallInfo)->rhs);
		if (!rhs) {
			OBJECT_DECREF(interp, lhs);
			return NULL;
		}

		enum Operator op = INFO_AS(AstOpCallInfo)->op ;
		struct Object *res = operator_call(interp, op, lhs, rhs);
		OBJECT_DECREF(interp, lhs);
		OBJECT_DECREF(interp, rhs);
		return res;   // may be NULL
	}

	if (nodedata->kind == AST_ARRAY) {
		struct Object *result = arrayobject_newwithcapacity(interp, ARRAYOBJECT_LEN(INFO_AS(AstArrayOrBlockInfo)));
		if (!result)
			return NULL;

		for (size_t i=0; i < ARRAYOBJECT_LEN(INFO_AS(AstArrayOrBlockInfo)); i++) {
			struct Object *elem = runast_expression(interp, scope, ARRAYOBJECT_GET(INFO_AS(AstArrayOrBlockInfo), i));
			if (!elem) {
				OBJECT_DECREF(interp, result);
				return NULL;
			}
			bool pushret = arrayobject_push(interp, result, elem);
			OBJECT_DECREF(interp, elem);
			if (!pushret) {
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

bool runast_statement(struct Interpreter *interp, struct Object *scope, struct Object *stmtnode)
{
	struct AstNodeObjectData *nodedata = stmtnode->objdata.data;

#define INFO_AS(X) ((struct X *) nodedata->info)
	if (nodedata->kind == AST_CALL) {
		struct Object *ret = runast_expression(interp, scope, stmtnode);
		if (!ret)
			return false;

		OBJECT_DECREF(interp, ret);  // ignore the return value
		return true;
	}

	if (nodedata->kind == AST_CREATEVAR) {
		struct Object *val = runast_expression(interp, scope, INFO_AS(AstCreateOrSetVarInfo)->valnode);
		if (!val)
			return false;

		bool res = mappingobject_set(interp, SCOPEOBJECT_LOCALVARS(scope), INFO_AS(AstCreateOrSetVarInfo)->varname, val);
		OBJECT_DECREF(interp, val);
		return res;
	}

	if (nodedata->kind == AST_SETVAR) {
		struct Object *val = runast_expression(interp, scope, INFO_AS(AstCreateOrSetVarInfo)->valnode);
		if (!val)
			return false;

		bool ok = scopeobject_setvar(interp, scope, INFO_AS(AstCreateOrSetVarInfo)->varname, val);
		OBJECT_DECREF(interp, val);
		return ok;
	}

	if (nodedata->kind == AST_SETATTR) {
		struct Object *obj = runast_expression(interp, scope, INFO_AS(AstSetAttrInfo)->objnode);
		if (!obj)
			return false;

		struct Object *val = runast_expression(interp, scope, INFO_AS(AstSetAttrInfo)->valnode);
		if (!val) {
			OBJECT_DECREF(interp, obj);
			return false;
		}

		bool res = attribute_setwithstringobj(interp, obj, INFO_AS(AstSetAttrInfo)->attr, val);
		OBJECT_DECREF(interp, obj);
		OBJECT_DECREF(interp, val);
		return res;
	}
#undef INFO_AS

	assert(0);
}
