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
		int res = operator_neint(interp, lhs, rhs);
		if (res == -1)
			return NULL;
		return boolobject_get(interp, res);
	}

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

int operator_eqint(struct Interpreter *interp, struct Object *a, struct Object *b)
{
	if (!interp->oparrays.eq)
	{
		// called early from builtins_setup()
		assert(a->klass == interp->builtins.String);
		assert(b->klass == interp->builtins.String);

		struct UnicodeString *astr = a->data;
		struct UnicodeString *bstr = b->data;

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

	struct Object *res = operator_call(interp, OPERATOR_EQ, a, b);
	int ret;
	if (!res)
		ret = -1;
	else if (res == interp->builtins.yes)
		ret = 1;
	else if (res == interp->builtins.no)
		ret = 0;
	else {
		errorobject_throwfmt(interp, "TypeError", "expected a Bool from %U == %U, but got %D", class_name(a), class_name(b), res);
		ret = -1;
	}

	if (res)
		OBJECT_DECREF(interp, res);
	return ret;
}

int operator_neint(struct Interpreter *interp, struct Object *a, struct Object *b)
{
	int res = operator_eqint(interp, a, b);
	return res<0 ? res : !res;
}
