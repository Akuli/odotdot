#include "equals.h"
#include "objects/array.h"
#include "objects/classobject.h"
#include "objects/mapping.h"
#include "objects/string.h"


int equals(struct Interpreter *interp, struct Object *a, struct Object *b)
{
	if (a == b)      // the SAME object, == compares pointers
		return 1;

	if (classobject_isinstanceof(a, interp->builtins.stringclass) && classobject_isinstanceof(b, interp->builtins.stringclass)) {
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

	if (classobject_isinstanceof(a, interp->builtins.integerclass) && classobject_isinstanceof(b, interp->builtins.integerclass)) {
		long long *aval = a->data, *bval = b->data;
		return (*aval == *bval);
	}

	if (classobject_isinstanceof(a, interp->builtins.arrayclass) && classobject_isinstanceof(b, interp->builtins.arrayclass)) {
		if (ARRAYOBJECT_LEN(a) != ARRAYOBJECT_LEN(b))
			return 0;
		for (size_t i=0; i < ARRAYOBJECT_LEN(a); i++) {
			int res = equals(interp, ARRAYOBJECT_GET(a, i), ARRAYOBJECT_GET(b, i));
			if (res != 1)    // -1 for error or 0 for not qeual
				return res;
		}
		return 1;
	}

	if (classobject_isinstanceof(a, interp->builtins.mappingclass) && classobject_isinstanceof(b, interp->builtins.mappingclass)) {
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
			res = equals(interp, iter.value, bval);
			OBJECT_DECREF(interp, bval);
			if (res != 1)
				return res;    // -1 for error or 0 for not equal
		}
		return 1;
	}

	// TODO: is this a good default? an error for 1 == "1" might make debugging easier
	return 0;
}
