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

// these are sane IMO
#define NARGS_MAX 20
#define NOPTS_MAX 50


bool check_type(struct Interpreter *interp, struct Object *klass, struct Object *obj)
{
	if (!classobject_isinstanceof(obj, klass)) {
		struct UnicodeString name = ((struct ClassObjectData*) klass->data)->name;
		errorobject_setwithfmt(interp, "should be an instance of %U, not %D", name, obj);
		return false;
	}
	return true;
}

bool check_args(struct Interpreter *interp, struct Object *args, ...)
{
	va_list ap;
	va_start(ap, args);

	unsigned int expectnargs;   // expected number of arguments
	struct Object *classes[NARGS_MAX];
	for (expectnargs=0; expectnargs < NARGS_MAX; expectnargs++) {
		struct Object *klass = va_arg(ap, struct Object *);
		if (!klass)
			break;      // end of argument list, not an error
		classes[expectnargs] = klass;
	}
	va_end(ap);

	// TODO: test these
	// TODO: include the function name in the error?
	if (ARRAYOBJECT_LEN(args) != expectnargs) {
		errorobject_setwithfmt(interp, "%s arguments", ARRAYOBJECT_LEN(args) > expectnargs ? "too many" : "not enough");
		return false;
	}

	for (unsigned int i=0; i < expectnargs; i++) {
		if (!check_type(interp, classes[i], ARRAYOBJECT_GET(args, i)))
			return false;
	}

	return true;
}

struct Opt {
	struct Object *klass;
	struct Object *name;
	struct Object **valptr;
};

bool check_opts(struct Interpreter *interp, struct Object *opts, ...)
{
	va_list ap;
	va_start(ap, opts);

	unsigned int nopts;
	struct Opt optinfos[NOPTS_MAX];
	for (nopts = 0; nopts < NOPTS_MAX; nopts++) {
		optinfos[nopts].klass = va_arg(ap, struct Object *);
		if (!optinfos[nopts].klass)
			break;      // end of argument list, not an error
		char *name = va_arg(ap, char *);
		optinfos[nopts].name = stringobject_newfromcharptr(interp, name);
		if (!optinfos[nopts].name)
			goto error;
		optinfos[nopts].valptr = va_arg(ap, struct Object **);
	}
	va_end(ap);

	// create a mapping with option names as both keys and
	// "contains key" checks are fast with mappings
	struct Object *validoptnames = mappingobject_newempty(interp);
	if (!validoptnames)
		goto error;

	for (size_t i=0; i < nopts; i++) {
		if (!mappingobject_set(interp, validoptnames, optinfos[i].name, optinfos[i].name)) {
			OBJECT_DECREF(interp, validoptnames);
			goto error;
		}
	}

	// make sure that opts doesn't contain anything not in validoptnames
	struct MappingObjectIter iter;
	mappingobject_iterbegin(&iter, opts);
	while (mappingobject_iternext(&iter)) {
		struct Object *dummy;
		int status = mappingobject_get(interp, validoptnames, iter.key, &dummy);
		if (status == 1) {
			// found
			OBJECT_DECREF(interp, dummy);
		} else {
			if (status == 0)    // not found from valid keys
				errorobject_setwithfmt(interp, "unexpected option %D", iter.key);
			OBJECT_DECREF(interp, validoptnames);
			goto error;
		}
	}

	OBJECT_DECREF(interp, validoptnames);

	// get values for all options
	for (unsigned int i=0; i < nopts; i++) {
		struct Object *val;
		int status = mappingobject_get(interp, opts, optinfos[i].name, &val);
		if (status == -1)
			goto error;
		if (status == 1) {
			// val was set to a new reference
			// but we can assume that opts holds the reference anyway
			// so let's delete the new reference
			OBJECT_DECREF(interp, val);

			if (!check_type(interp, optinfos[i].klass, val))
				goto error;
		}
		*optinfos[i].valptr = val;
	}

	return true;

error:
	for (unsigned int i=0; i < nopts; i++)
		OBJECT_DECREF(interp, optinfos[i].name);
	return false;
}
