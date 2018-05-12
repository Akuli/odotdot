// TODO: implement bignums? maybe a pure-ö BigInteger class?
// TODO: better error handling than asserts

#include "integer.h"
#include <assert.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include "classobject.h"
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

static struct Object *to_string(struct Context *ctx, struct Object **errptr, struct Object **args, size_t nargs)
{
	if (functionobject_checktypes(ctx, errptr, args, nargs, "Integer", NULL) == STATUS_ERROR)
		return NULL;

	int64_t val = *((int64_t *) args[0]->data);
	if (val == 0)   // special case
		return stringobject_newfromcharptr(ctx->interp, errptr, "0");

	uint64_t absval;
	if (val < 0) {
		// '-' will be added to res later
		// careful here... -INT64_MIN overflows, but -(INT64_MIN+1) doesn't
		// uint64_t can hold anything
		absval = (uint64_t)(-(val+1)) + 1;
	} else {
		absval = val;
	}

	char res[INTEGER_MAXLEN+1];
	res[INTEGER_MAXLEN] = 0;
	int i = INTEGER_MAXLEN;
	while( absval != 0) {
		res[--i] = (char)(absval % 10) + '0';
		absval /= 10;
	}
	if (val < 0)
		res[--i] = '-';

	return stringobject_newfromcharptr(ctx->interp, errptr, res+i);
}

struct Object *integerobject_createclass(struct Interpreter *interp, struct Object **errptr)
{
	struct Object *objectclass = interpreter_getbuiltin(interp, errptr, "Object");
	if (!objectclass)
		return NULL;

	struct Object *klass = classobject_new(interp, errptr, "Integer", objectclass, 0, NULL, integer_destructor);
	OBJECT_DECREF(interp, objectclass);
	if (!klass)
		return NULL;

	if (method_add(interp, errptr, klass, "to_string", to_string) == STATUS_ERROR) {
		OBJECT_DECREF(interp, klass);
		return NULL;
	}
	return klass;
}

// TODO: take a context and use errorobject_setwithfmt instead
static struct Object *integer_from_digits(struct Interpreter *interp, struct Object **errptr, int isnegative, int *digits, int ndigits)
{
	struct Object *integerclass = interpreter_getbuiltin(interp, errptr, "Integer");
	if (!integerclass)
		return NULL;

	assert(ndigits > 0);

	// uint64_t can handle these because it doesn't use one bit for the sign
	uint64_t absval = 0;

	// careful here... -INT64_MIN doesn't fit in an int64_t, but -(INT64_MIN+1) fits
	// everything fits into an uint64_t
#define ABSMAX (isnegative ? (uint64_t)(-(INT64_MIN + 1)) + 1 : (uint64_t)INT64_MAX)

	uint64_t multipleby = 1;
	for (int i=ndigits-1; i>=0; i--) {
		// error condition:
		//    absval + digits[i]*multipleby > ABSMAX
		//    absval > ABSMAX - digits[i]*multipleby
		assert(absval <= ABSMAX - digits[i]*multipleby);
		absval += digits[i]*multipleby;
		multipleby *= 10;
	}
#undef ABSMAX

	int64_t *data = malloc(sizeof(int64_t));
	if (!data)
		return NULL;
	*data = isnegative ? -absval : absval;

	struct Object *integer = classobject_newinstance(interp, errptr, integerclass, data);
	OBJECT_DECREF(interp, integerclass);
	if (!integer) {
		free(data);
		return NULL;
	}
	return integer;
}

struct Object *integerobject_newfromustr(struct Interpreter *interp, struct Object **errptr, struct UnicodeString ustr)
{
	int isnegative=(ustr.val[0] == '-');
	if (isnegative) {
		// this relies on pass-by-value
		ustr.val++;
		ustr.len--;
	}

	// remove leading zeros, but leave a single digit zero alone
	// e.g. 0000000123 ==> 123, 00000000 ==> 0, 000001 ==> 1
	while (ustr.len > 1 && ustr.val[0] == '0') {
		ustr.val++;
		ustr.len--;
	}

	// TODO: better error handling
	assert(1 <= ustr.len && ustr.len <= INTEGER_MAXLEN);

	int digits[INTEGER_MAXLEN];
	for (int i=0; i < (int)ustr.len; i++) {
		assert('0' <= ustr.val[i] && ustr.val[i] <= '9');
		digits[i] = ustr.val[i] - '0';
	}
	return integer_from_digits(interp, errptr, isnegative, digits, ustr.len);
}

// this is a lot like newfromustr, see above
struct Object *integerobject_newfromcharptr(struct Interpreter *interp, struct Object **errptr, char *s)
{
	int isnegative=(s[0] == '-');
	if (isnegative)
		s++;

	size_t ndigits = strlen(s);
	while (ndigits > 1 && s[0] == '0') {
		s++;
		ndigits--;
	}
	assert(1 <= ndigits && ndigits <= INTEGER_MAXLEN);

	int digits[INTEGER_MAXLEN];
	for (int i=0; i < (int)ndigits; i++) {
		assert('0' <= s[i] && s[i] <= '9');
		digits[i] = s[i] - '0';
	}
	return integer_from_digits(interp, errptr, isnegative, digits, ndigits);
}

int64_t integerobject_toint64(struct Object *integer)
{
	return *((int64_t *) integer->data);
}
