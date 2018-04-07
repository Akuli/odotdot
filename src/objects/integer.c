// TODO: implement bignums? maybe a pure-ö BigInteger class?
// TODO: better error handling than asserts

#include "integer.h"
#include <assert.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include "classobject.h"
#include "../common.h"
#include "../objectsystem.h"
#include "../unicode.h"

static void integer_destructor(struct Object *integer)
{
	free(integer->data);
}

int integerobject_createclass(struct Interpreter *interp, struct Object **errptr)
{
	struct Object *objectclass = interpreter_getbuiltin(interp, errptr, "Object");
	if (!objectclass)
		return STATUS_ERROR;

	struct Object *klass = classobject_new(interp, errptr, "Integer", objectclass, NULL, integer_destructor);
	OBJECT_DECREF(interp, objectclass);
	if (!klass)
		return STATUS_ERROR;

	int ret = interpreter_addbuiltin(interp, errptr, "Integer", klass);
	OBJECT_DECREF(interp, klass);
	return ret;
}

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
	// 64-bit ints are never 25 digits or more long
	// TODO: better error handling
	assert(1 <= ustr.len && ustr.len < 25);
	int isnegative=(ustr.val[0] == '-');
	if (isnegative) {
		// this relies on pass-by-value
		ustr.val++;
		ustr.len--;
		assert(ustr.len >= 1);
	}

	int digits[25];
	for (int i=0; i < (int)ustr.len; i++) {
		assert('0' <= ustr.val[i] && ustr.val[i] <= '9');
		digits[i] = ustr.val[i] - '0';
	}
	return integer_from_digits(interp, errptr, isnegative, digits, ustr.len);
}

struct Object *integerobject_newfromcharptr(struct Interpreter *interp, struct Object **errptr, char *s)
{
	int isnegative=(s[0] == '-');
	if (isnegative)
		s++;

	size_t ndigits = strlen(s);
	assert(ndigits != 0);
	assert(ndigits < 25);

	int digits[25];
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
