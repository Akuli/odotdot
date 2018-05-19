// TODO: implement bignums? maybe a pure-ö BigInteger class?
// TODO: better error handling than asserts

#include "integer.h"
#include <assert.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include "classobject.h"
#include "errors.h"
#include "function.h"
#include "string.h"
#include "../common.h"
#include "../context.h"
#include "../interpreter.h"
#include "../method.h"
#include "../objectsystem.h"
#include "../unicode.h"


static void integer_destructor(struct Object *integer)
{
	free(integer->data);
}

static struct Object *to_string(struct Interpreter *interp, struct Object **errptr, struct Object **args, size_t nargs)
{
	if (functionobject_checktypes(interp, errptr, args, nargs, interp->builtins.integerclass, NULL) == STATUS_ERROR)
		return NULL;

	long long val = *((long long *) args[0]->data);
	if (val == 0)   // special case
		return stringobject_newfromcharptr(interp, errptr, "0");

	assert(INTEGEROBJECT_MIN <= val && val <= INTEGEROBJECT_MAX);

	unsigned long long absval;   // 1 more bit of magnitude than long long
	if (val < 0) {
		// '-' will be added to res later
		// careful here... -INTEGEROBJECT_MIN overflows, but -(INTEGEROBJECT_MIN+1) doesn't
		absval = ( (unsigned long long)(-(val+1)) ) + 1;
	} else {
		absval = val;
	}

	// res is filled in backwards because it's handy
	char res[INTEGEROBJECT_MAXDIGITS+1];
	res[INTEGEROBJECT_MAXDIGITS] = 0;
	int i = INTEGEROBJECT_MAXDIGITS;
	while( absval != 0) {
		res[--i] = (char)(absval % 10) + '0';
		absval /= 10;
	}
	if (val < 0)
		res[--i] = '-';

	return stringobject_newfromcharptr(interp, errptr, res+i);
}

struct Object *integerobject_createclass(struct Interpreter *interp, struct Object **errptr)
{
	struct Object *klass = classobject_new(interp, errptr, "Integer", interp->builtins.objectclass, 0, NULL);
	if (!klass)
		return NULL;

	if (method_add(interp, errptr, klass, "to_string", to_string) == STATUS_ERROR) goto error;
	return klass;

error:
	OBJECT_DECREF(interp, klass);
	return NULL;
}

struct Object *integerobject_newfromlonglong(struct Interpreter *interp, struct Object **errptr, long long val)
{
	assert(INTEGEROBJECT_MIN <= val && val <= INTEGEROBJECT_MAX);

	long long *data = malloc(sizeof(long long));
	if (!data) {
		errorobject_setnomem(interp, errptr);
		return NULL;
	}
	*data = val;

	struct Object *integer = classobject_newinstance(interp, errptr, interp->builtins.integerclass, data, integer_destructor);
	if (!integer) {
		free(data);
		return NULL;
	}
	integer->hash = (unsigned int) val;
	return integer;
}

struct Object *integerobject_newfromustr(struct Interpreter *interp, struct Object **errptr, struct UnicodeString ustr)
{
	struct UnicodeString origstr = ustr;

	int isnegative=(ustr.val[0] == '-');
	if (isnegative) {
		// this relies on pass-by-value
		ustr.val++;
		ustr.len--;
	}

	// remove leading zeros, but leave a single digit zero alone
	// e.g. 0000000123 --> 123, 00000000 --> 0, 000001 --> 1
	while (ustr.len > 1 && ustr.val[0] == '0') {
		ustr.val++;
		ustr.len--;
	}

	if (ustr.len == 0) {
		errorobject_setwithfmt(interp, errptr, "'%U' is not an integer", origstr);
		return NULL;
	}
	if (ustr.len > INTEGEROBJECT_MAXDIGITS)
		goto mustBbetween;

	int digits[INTEGEROBJECT_MAXDIGITS];
	for (int i=0; i < (int)ustr.len; i++) {
		if (ustr.val[i] < '0' || ustr.val[i] > '9') {
			errorobject_setwithfmt(interp, errptr, "'%U' is not an integer", origstr);
			return NULL;
		}
		digits[i] = ustr.val[i] - '0';
	}

	// see to_string
	unsigned long long absval = 0;
#define ABSMAX (isnegative ? ( (unsigned long long)(-(INTEGEROBJECT_MIN+1)) ) + 1 : (unsigned long long)INTEGEROBJECT_MAX)

	unsigned long long multipleby = 1;
	for (int i = ustr.len-1; i>=0; i--) {
		if (absval > ABSMAX - digits[i]*multipleby)
			goto mustBbetween;
		absval += digits[i]*multipleby;   // fitting was just checked
		multipleby *= 10;
	}
#undef ABSMAX

	long long val = isnegative ? -((long long)(absval-1)) - 1 : (long long)absval;
	return integerobject_newfromlonglong(interp, errptr, val);

mustBbetween:
	errorobject_setwithfmt(interp, errptr, "integers must be between %s and %s, but '%U' is not", INTEGEROBJECT_MINSTR, INTEGEROBJECT_MAXSTR, origstr);
	return NULL;
}

struct Object *integerobject_newfromcharptr(struct Interpreter *interp, struct Object **errptr, char *s)
{
	// it would be easy to skip many zeros like this:   while (s[0] == '0') s++;
	// but -000000000000000000000000000000001 must be turned into -1
	// it would be possible to do this without another malloc, but performance-critical
	// stuff should use newfromustr or newfromlonglong anyway
	struct UnicodeString ustr;
	ustr.len = strlen(s);
	ustr.val = malloc(sizeof(unicode_char) * ustr.len);
	if (!ustr.val) {
		errorobject_setnomem(interp, errptr);
		return NULL;
	}

	// can't memcpy because types differ
	for (size_t i=0; i < ustr.len; i++)
		ustr.val[i] = s[i];

	struct Object *res = integerobject_newfromustr(interp, errptr, ustr);
	free(ustr.val);
	return res;
}

long long integerobject_tolonglong(struct Object *integer)
{
	return *((long long *) integer->data);
}
