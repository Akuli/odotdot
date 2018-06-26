#include "operator.h"
#include <assert.h>
#include "check.h"
#include "interpreter.h"
#include "objectsystem.h"
#include "objects/array.h"
#include "objects/bool.h"
#include "objects/classobject.h"
#include "objects/errors.h"
#include "objects/function.h"
#include "objects/mapping.h"
#include "unicode.h"

#define class_name(obj) (((struct ClassObjectData *) (obj)->klass->data)->name)


struct Object *operator_call(struct Interpreter *interp, enum Operator op, struct Object *lhs, struct Object *rhs)
{
	if (op == OPERATOR_NE) {
		int res = operator_eqint(interp, lhs, rhs);
		return res<0 ? NULL : boolobject_get(interp, !res);
	}
	if (op == OPERATOR_LE) {
		int val1, val2;
		if ((val1 = operator_ltint(interp, lhs, rhs)) < 0)
			return NULL;
		if ((val2 = operator_eqint(interp, lhs, rhs)) < 0)
			return NULL;
		return boolobject_get(interp, val1||val2);
	}

	// these can be implemented with other stuff by flipping lhs and rhs
	if (op == OPERATOR_GT)
		return operator_call(interp, OPERATOR_LT, rhs, lhs);
	if (op == OPERATOR_GE)
		return operator_call(interp, OPERATOR_LE, rhs, lhs);

	struct Object *oparray;
	char *opstr;
	if (op == OPERATOR_ADD) {
		oparray = interp->oparrays.add;
		opstr = "+";
	} else if (op == OPERATOR_SUB) {
		oparray = interp->oparrays.sub;
		opstr = "-";
	} else if (op == OPERATOR_MUL) {
		oparray = interp->oparrays.mul;
		opstr = "*";
	} else if (op == OPERATOR_DIV) {
		oparray = interp->oparrays.div;
		opstr = "/";
	} else if (op == OPERATOR_EQ) {
		oparray = interp->oparrays.eq;
		opstr = "==";
	} else if (op == OPERATOR_LT) {
		oparray = interp->oparrays.lt;
		opstr = "<";
	} else
		assert(0);

	for (size_t i=0; i < ARRAYOBJECT_LEN(oparray); i++) {
		struct Object *func = ARRAYOBJECT_GET(oparray, i);
		if (!check_type(interp, interp->builtins.Function, func))
			return NULL;

		struct Object *res = functionobject_call(interp, func, lhs, rhs, NULL);
		if (res == interp->builtins.null) {
			OBJECT_DECREF(interp, res);
			continue;
		}
		return res;   // NULL or a new reference
	}

	// nothing matching found from the oparray
	errorobject_throwfmt(interp, "TypeError", "%U %s %U", class_name(lhs), opstr, class_name(rhs));
	return NULL;
}

int operator_eqint(struct Interpreter *interp, struct Object *lhs, struct Object *rhs)
{
	if (!interp->oparrays.eq)
	{
		// called early from builtins_setup()
		assert(lhs->klass == interp->builtins.String);
		assert(rhs->klass == interp->builtins.String);

		struct UnicodeString *astr = lhs->data;
		struct UnicodeString *bstr = rhs->data;

		if (astr->len != bstr->len)
			return 0;

		// memcmp isn't reliable :( grep for memcmp in the source for detailz
		for (size_t i=0; i < astr->len; ++i)
		{
			if (astr->val[i]!=bstr->val[i])
			 return 0;
		}
		return 1;
	}

	struct Object *res = operator_call(interp, OPERATOR_EQ, lhs, rhs);
	int ret;
	if (!res)
		ret = -1;
	else if (res == interp->builtins.yes)
		ret = 1;
	else if (res == interp->builtins.no)
		ret = 0;
	else {
		errorobject_throwfmt(interp, "TypeError", "expected a Bool from %U == %U, but got %D", class_name(lhs), class_name(rhs), res);
		ret = -1;
	}

	if (res)
		OBJECT_DECREF(interp, res);
	return ret;
}

int operator_ltint(struct Interpreter *interp, struct Object *lhs, struct Object *rhs)
{
	struct Object *res = operator_call(interp, OPERATOR_LT, lhs, rhs);
	int ret;
	if (!res)
		ret = -1;
	else if (res == interp->builtins.yes)
		ret = 1;
	else if (res == interp->builtins.no)
		ret = 0;
	else {
		errorobject_throwfmt(interp, "TypeError", "expected a Bool from %U < %U, but got %D", class_name(lhs), class_name(rhs), res);
		ret = -1;
	}

	if (res)
		OBJECT_DECREF(interp, res);
	return ret;
}
