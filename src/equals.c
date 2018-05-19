#include "equals.h"
#include <assert.h>
#include "objects/array.h"
#include "objects/classobject.h"
#include "objects/string.h"


int equals(struct Interpreter *interp, struct Object **errptr, struct Object *a, struct Object *b)
{
	if (a == b)
		return 1;

	struct Object *stringclass = interpreter_getbuiltin(interp, errptr, "String");
	if (!stringclass)
		return -1;
	int arestrings = classobject_instanceof(a, stringclass) && classobject_instanceof(b, stringclass);
	OBJECT_DECREF(interp, stringclass);

	if (arestrings) {
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

	struct Object *integerclass = interpreter_getbuiltin(interp, errptr, "Integer");
	if (!integerclass)
		return -1;
	int areints = classobject_instanceof(a, integerclass) && classobject_instanceof(b, integerclass);
	OBJECT_DECREF(interp, integerclass);

	if (areints) {
		long long *aval = a->data, *bval = b->data;
		return (*aval == *bval);
	}

	struct Object *arrayclass = interpreter_getbuiltin(interp, errptr, "Array");
	if (!arrayclass)
		return -1;
	int arearrays = classobject_instanceof(a, arrayclass) && classobject_instanceof(b, arrayclass);
	OBJECT_DECREF(interp, arrayclass);

	if (arearrays) {
		struct ArrayObjectData *adata = a->data, *bdata = b->data;
		if (adata->len != bdata->len)
			return 0;
		for (size_t i=0; i < adata->len; i++) {
			int res = equals(interp, errptr, adata->elems[i], bdata->elems[i]);
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
