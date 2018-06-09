#include "check.h"
#include <stdarg.h>
#include <stdbool.h>
#include "objects/array.h"
#include "objects/classobject.h"
#include "objects/errors.h"


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
