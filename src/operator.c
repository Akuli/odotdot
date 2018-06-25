#include "operator.h"
#include <assert.h>
#include "check.h"
#include "interpreter.h"
#include "objectsystem.h"
#include "objects/array.h"
#include "objects/classobject.h"
#include "objects/errors.h"
#include "objects/function.h"
#include "objects/mapping.h"
#include "unicode.h"


struct Object *operator_call(struct Interpreter *interp, enum Operator op, struct Object *lhs, struct Object *rhs)
{
	if (op == OPERATOR_EQ || op == OPERATOR_NE) {
		int res = (op == OPERATOR_EQ)? operator_eqint(interp, lhs, rhs) : operator_neint(interp, lhs, rhs);
		if (res < 0)
			return NULL;
		return interpreter_getbuiltin(interp, res ? "true" : "false");
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
#define class_name(obj) (((struct ClassObjectData *) (obj)->klass->data)->name)
	errorobject_throwfmt(interp, "TypeError", "%U %s %U", class_name(lhs), opstr, class_name(rhs));
#undef class_name
	return NULL;
}

// TODO: use an oparray instead
int operator_eqint(struct Interpreter *interp, struct Object *a, struct Object *b)
{
	// mostly for optimization
	if (a == b)
		return 1;

	if (classobject_isinstanceof(a, interp->builtins.String) && classobject_isinstanceof(b, interp->builtins.String)) {
		struct UnicodeString *astr = a->data, *bstr = b->data;
		if (astr->len != bstr->len)
			return 0;

		// memcmp is not reliable :( https://stackoverflow.com/a/11995514
		for (size_t i=0; i < astr->len; i++) {
			if (astr->val[i] != bstr->val[i])
				return 0;
		}
		return 1;
	}

	if (classobject_isinstanceof(a, interp->builtins.Integer) && classobject_isinstanceof(b, interp->builtins.Integer)) {
		long long *aval = a->data, *bval = b->data;
		return (*aval == *bval);
	}

	if (classobject_isinstanceof(a, interp->builtins.Array) && classobject_isinstanceof(b, interp->builtins.Array)) {
		if (ARRAYOBJECT_LEN(a) != ARRAYOBJECT_LEN(b))
			return 0;
		for (size_t i=0; i < ARRAYOBJECT_LEN(a); i++) {
			int res = operator_eqint(interp, ARRAYOBJECT_GET(a, i), ARRAYOBJECT_GET(b, i));
			if (res != 1)    // -1 for error or 0 for not qeual
				return res;
		}
		return 1;
	}

	if (classobject_isinstanceof(a, interp->builtins.Mapping) && classobject_isinstanceof(b, interp->builtins.Mapping)) {
		// mappings are equal if they have same keys and values
		// that's true if and only if for every key of a, a.get(key) == b.get(key)
		// unless a and b are of different lengths
		// or a and b have same number of keys but different keys, and .get() fails

		if (MAPPINGOBJECT_SIZE(a) != MAPPINGOBJECT_SIZE(b))
			return 0;

		struct MappingObjectIter iter;
		mappingobject_iterbegin(&iter, a);
		while (mappingobject_iternext(&iter)) {
			struct Object *bval;

			int res = mappingobject_get(interp, b, iter.key, &bval);
			if (res != 1)
				return res;   // -1 for error or 0 for key not found

			// ok, so we found the value... let's compare
			res = operator_eqint(interp, iter.value, bval);
			OBJECT_DECREF(interp, bval);
			if (res != 1)
				return res;    // -1 for error or 0 for not equal
		}
		return 1;
	}

	// TODO: is this a good default? an error for 1 == "1" might make debugging easier
	return 0;
}

int operator_neint(struct Interpreter *interp, struct Object *a, struct Object *b)
{
	int res = operator_eqint(interp, a, b);
	if (res == -1)
		return -1;
	return !res;
}
