#include "equals.h"
#include <assert.h>
#include "objects/array.h"
#include "objects/classobject.h"
#include "objects/string.h"


int equals(struct Interpreter *interp, struct Object *a, struct Object *b)
{
	if (a == b)      // the SAME object, == compares pointers
		return 1;

	if (classobject_instanceof(a, interp->builtins.stringclass) && classobject_instanceof(b, interp->builtins.stringclass)) {
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

	if (classobject_instanceof(a, interp->builtins.integerclass) && classobject_instanceof(b, interp->builtins.integerclass)) {
		long long *aval = a->data, *bval = b->data;
		return (*aval == *bval);
	}

	if (classobject_instanceof(a, interp->builtins.arrayclass) && classobject_instanceof(b, interp->builtins.arrayclass)) {
		if (ARRAYOBJECT_LEN(a) != ARRAYOBJECT_LEN(b))
			return 0;
		for (size_t i=0; i < ARRAYOBJECT_LEN(a); i++) {
			int res = equals(interp, ARRAYOBJECT_GET(a, i), ARRAYOBJECT_GET(b, i));
			if (res == -1)
				return -1;
			if (res == 0)
				return 0;
			assert(res == 1);
		}
		return 1;
	}

	// TODO: is this a good default? an error for 1 == "1" might make debugging easier
	return 0;
}
