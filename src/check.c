#include "check.h"
#include <assert.h>
#include <stdarg.h>
#include <stdbool.h>
#include <string.h>
#include "objects/array.h"
#include "objects/classobject.h"
#include "objects/errors.h"
#include "objects/mapping.h"
#include "objects/null.h"
#include "objects/string.h"
#include "unicode.h"


bool check_type(struct Interpreter *interp, struct Object *klass, struct Object *obj)
{
	if (!classobject_isinstanceof(obj, klass)) {
		struct UnicodeString name = ((struct ClassObjectData*) klass->objdata.data)->name;
		errorobject_throwfmt(interp, "TypeError", "expected an instance of %U, got %D", name, obj);
		return false;
	}
	return true;
}

bool check_args_with_array(struct Interpreter *interp, struct Object *args, struct Object *types)
{
	// TODO: include the function name in the error?
	if (ARRAYOBJECT_LEN(args) != ARRAYOBJECT_LEN(types)) {
		errorobject_throwfmt(interp, "ArgError", "%s arguments: expected %L, got %L",
			ARRAYOBJECT_LEN(args)>ARRAYOBJECT_LEN(types) ? "too many" : "not enough",
			(long long) ARRAYOBJECT_LEN(types), (long long) ARRAYOBJECT_LEN(args));
		return false;
	}

	for (size_t i=0; i < ARRAYOBJECT_LEN(args); i++) {
		if (!check_type(interp, ARRAYOBJECT_GET(types, i), ARRAYOBJECT_GET(args, i)))
			return false;
	}
	return true;
}

bool check_args(struct Interpreter *interp, struct Object *args, ...)
{
	struct Object *types = arrayobject_newempty(interp);
	if (!types)
		return false;

	va_list ap;
	va_start(ap, args);
	while(true) {
		struct Object *klass = va_arg(ap, struct Object *);
		if (!klass)
			break;      // end of argument list, not an error
		if (!arrayobject_push(interp, types, klass))
			return false;
	}
	va_end(ap);

	bool ok = check_args_with_array(interp, args, types);
	OBJECT_DECREF(interp, types);
	return ok;
}

bool check_opts_with_mapping(struct Interpreter *interp, struct Object *opts, struct Object *types)
{
	struct MappingObjectIter iter;
	mappingobject_iterbegin(&iter, opts);

	while (mappingobject_iternext(&iter)) {
		struct Object *klass;
		int status = mappingobject_get(interp, types, iter.key, &klass);
		if (status == 1) {
			// found
			bool ok = check_type(interp, klass, iter.value);
			OBJECT_DECREF(interp, klass);
			if (!ok)
				return false;
		} else {
			if (status == 0)    // not found
				errorobject_throwfmt(interp, "ArgError", "unexpected option %D", iter.key);
			return false;
		}
	}

	return true;
}

bool check_opts(struct Interpreter *interp, struct Object *opts, ...)
{
	struct Object *types = NULL;   // optimization for checking that there are no options

	va_list ap;
	va_start(ap, opts);

	while(true){
		struct Object *nameobj = va_arg(ap, struct Object *);
		if (!nameobj)
			break;      // end of argument list, not an error

		struct Object *klass = va_arg(ap, struct Object *);
		assert(klass);

		if (!types) {
			types = mappingobject_newempty(interp);
			if (!types)
				return false;
		}

		if (!mappingobject_set(interp, types, nameobj, klass)) {
			OBJECT_DECREF(interp, types);
			return false;
		}
	}
	va_end(ap);

	if (!types) {
		// throw an error if there are any options
		if (MAPPINGOBJECT_SIZE(opts) == 0)
			return true;

		// pick 1 option for consistency with check_opts_with_mapping (lol)
		struct MappingObjectIter iter;
		mappingobject_iterbegin(&iter, opts);
		mappingobject_iternext(&iter);
		errorobject_throwfmt(interp, "ArgError", "unexpected option %D", iter.key);
		return false;
	}

	bool ok = check_opts_with_mapping(interp, opts, types);
	OBJECT_DECREF(interp, types);
	return ok;
}
