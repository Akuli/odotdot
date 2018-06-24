#include "runast.h"
#include <assert.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>
#include "attribute.h"
#include "check.h"
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
static struct Object *runast_expression(struct Interpreter *interp, struct Object *scope, struct Object *exprnode)
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

	if (nodedata->kind == AST_GETATTR) {
		struct Object *obj = runast_expression(interp, scope, INFO_AS(AstGetAttrInfo)->objnode);
		if (!obj)
			return NULL;

		struct Object *s = stringobject_newfromustr(interp, INFO_AS(AstGetAttrInfo)->name);
		if (!s) {
			OBJECT_DECREF(interp, obj);
			return NULL;
		}

		struct Object *res = attribute_getwithstringobj(interp, obj, s);
		OBJECT_DECREF(interp, obj);
		OBJECT_DECREF(interp, s);
		return res;
	}

	if (nodedata->kind == AST_CALL) {
		struct Object *func = runast_expression(interp, scope, INFO_AS(AstCallInfo)->funcnode);
		if (!func)
			return NULL;
		if (!check_type(interp, interp->builtins.Function, func)) {
			OBJECT_DECREF(interp, func);
			return NULL;
		}

		struct Object *args = arrayobject_newempty(interp);
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
		char op = INFO_AS(AstOpCallInfo)->op;
		struct Object *oparray;
		if (op == '+')
			oparray = interp->oparrays.add;
		else if (op == '-')
			oparray = interp->oparrays.sub;
		else if (op == '*')
			oparray = interp->oparrays.mul;
		else if (op == '/')
			oparray = interp->oparrays.div;
		else
			assert(0);

		struct Object *lhs = runast_expression(interp, scope, INFO_AS(AstOpCallInfo)->lhs);
		if (!lhs)
			return NULL;
		struct Object *rhs = runast_expression(interp, scope, INFO_AS(AstOpCallInfo)->rhs);
		if (!lhs) {
			OBJECT_DECREF(interp, lhs);
			return NULL;
		}

		for (size_t i=0; i < ARRAYOBJECT_LEN(oparray); i++) {
			struct Object *func = ARRAYOBJECT_GET(oparray, i);
			if (!check_type(interp, interp->builtins.Function, func)) {
				OBJECT_DECREF(interp, lhs);
				OBJECT_DECREF(interp, rhs);
				return NULL;
			}

			struct Object *res = functionobject_call(interp, func, lhs, rhs, NULL);
			if (res == interp->builtins.null) {
				OBJECT_DECREF(interp, res);
				continue;
			}

			OBJECT_DECREF(interp, lhs);
			OBJECT_DECREF(interp, rhs);
			return res;   // NULL or a new reference
		}
	}

	if (nodedata->kind == AST_ARRAY) {
		struct Object *result = arrayobject_newempty(interp);
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
	struct AstNodeObjectData *nodedata = stmtnode->data;

#define INFO_AS(X) ((struct X *) nodedata->info)
	if (nodedata->kind == AST_CALL) {
		struct Object *ret = runast_expression(interp, scope, stmtnode);
		if (!ret)
			return false;

		OBJECT_DECREF(interp, ret);  // ignore the return value
		return true;
	}

	if (nodedata->kind == AST_CREATEVAR) {
		// TODO: optimize ://(///((((
		struct Object *val = runast_expression(interp, scope, INFO_AS(AstCreateOrSetVarInfo)->valnode);
		if (!val)
			return false;

		struct Object *keystr = stringobject_newfromustr(interp, INFO_AS(AstCreateOrSetVarInfo)->varname);
		if (!keystr) {
			OBJECT_DECREF(interp, val);
		}

		bool res = mappingobject_set(interp, SCOPEOBJECT_LOCALVARS(scope), keystr, val);
		OBJECT_DECREF(interp, keystr);
		OBJECT_DECREF(interp, val);
		return res;
	}

	if (nodedata->kind == AST_SETVAR) {
		struct Object *namestr = stringobject_newfromustr(interp, INFO_AS(AstCreateOrSetVarInfo)->varname);
		if (!namestr)
			return false;

		struct Object *val = runast_expression(interp, scope, INFO_AS(AstCreateOrSetVarInfo)->valnode);
		if (!val) {
			OBJECT_DECREF(interp, namestr);
			return false;
		}

		// TODO: expose Scope.set_var?
		struct Object *res = method_call(interp, scope, "set_var", namestr, val, NULL);
		OBJECT_DECREF(interp, namestr);
		OBJECT_DECREF(interp, val);

		if (res) {
			OBJECT_DECREF(interp, res);
			return true;
		}
		return false;
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

		struct Object *stringobj = stringobject_newfromustr(interp, INFO_AS(AstSetAttrInfo)->attr);
		if (!stringobj) {
			OBJECT_DECREF(interp, val);
			OBJECT_DECREF(interp, obj);
			return false;
		}

		bool res = attribute_setwithstringobj(interp, obj, stringobj, val);
		OBJECT_DECREF(interp, obj);
		OBJECT_DECREF(interp, val);
		OBJECT_DECREF(interp, stringobj);
		return res;
	}
#undef INFO_AS

	assert(0);
}
