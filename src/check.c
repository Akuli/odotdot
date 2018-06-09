#include "check.h"
#include <stdarg.h>
#include <stdbool.h>
#include <string.h>
#include "objects/array.h"
#include "objects/classobject.h"
#include "objects/errors.h"
#include "objects/mapping.h"
#include "unicode.h"


bool check_type(struct Interpreter *interp, struct Object *klass, struct Object *obj)
{
	if (!classobject_isinstanceof(obj, klass)) {
		struct UnicodeString name = ((struct ClassObjectData*) klass->data)->name;
		errorobject_setwithfmt(interp, "should be an instance of %U, not %D", name, obj);
		return false;
	}
	return true;
}

#define NARGS_MAX 20    // sane
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

bool check_no_opts(struct Interpreter *interp, struct Object *opts)
{
	if (MAPPINGOBJECT_SIZE(opts) != 0) {
		// join the option names together with commaspaces
		size_t joinedlen = 0;
		struct MappingObjectIter iter;
		mappingobject_iterbegin(&iter, opts);
		while (mappingobject_iternext(&iter)) {
			joinedlen += ((struct UnicodeString*) iter.key->data)->len;
			// add commaspace once for each item except the first
			if (joinedlen != 0)
				joinedlen += 2;
		}

		unicode_char joinedval[joinedlen];
		unicode_char *ptr = joinedval;
		mappingobject_iterbegin(&iter, opts);
		while (mappingobject_iternext(&iter)) {
			struct UnicodeString *u = iter.key->data;
			memcpy(ptr, u->val, u->len * sizeof(unicode_char));
			ptr += u->len;
			if (ptr != joinedval+joinedlen) {   // not done yet
				*ptr++ = ',';
				*ptr++ = ' ';
			}
		}

		// phewh... almost done
		struct UnicodeString joined = { .len = joinedlen, .val = joinedval };
		errorobject_setwithfmt(interp, "expected no options, got %d option%s: %U",
			MAPPINGOBJECT_SIZE(opts), MAPPINGOBJECT_SIZE(opts)==1 ? "" : "s", joined);
		return false;
	}

	return true;
}
