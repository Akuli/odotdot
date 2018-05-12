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

static struct Object *to_string(struct Context *ctx, struct Object **errptr, struct Object **args, size_t nargs)
{
	if (functionobject_checktypes(ctx, errptr, args, nargs, "Integer", NULL) == STATUS_ERROR)
		return NULL;

	long long val = *((long long *) args[0]->data);
	if (val == 0)   // special case
		return stringobject_newfromcharptr(ctx->interp, errptr, "0");

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

struct Object *integerobject_newfromlonglong(struct Interpreter *interp, struct Object **errptr, long long val)
{
	assert(INTEGEROBJECT_MIN <= val && val <= INTEGEROBJECT_MAX);

	long long *data = malloc(sizeof(long long));
	if (!data)
		return NULL;
	*data = val;

	struct Object *integerclass = interpreter_getbuiltin(interp, errptr, "Integer");
	if (!integerclass) {
		free(data);
		return NULL;
	}

	struct Object *integer = classobject_newinstance(interp, errptr, integerclass, data);
	OBJECT_DECREF(interp, integerclass);
	if (!integer) {
		free(data);
		return NULL;
	}
	return integer;
}

// TODO: take a context and use errorobject_setwithfmt instead
static struct Object *integer_from_digits(struct Interpreter *interp, struct Object **errptr, int isnegative, int *digits, int ndigits)
{
	assert(ndigits > 0);

	// see to_string
	unsigned long long absval = 0;
#define ABSMAX (isnegative ? ( (unsigned long long)(-(INTEGEROBJECT_MIN+1)) ) + 1 : (unsigned long long)INTEGEROBJECT_MAX)

	unsigned long long multipleby = 1;
	for (int i=ndigits-1; i>=0; i--) {
		// error condition:
		//    absval + digits[i]*multipleby > ABSMAX
		//    absval > ABSMAX - digits[i]*multipleby
		assert(absval <= ABSMAX - digits[i]*multipleby);
		absval += digits[i]*multipleby;
		multipleby *= 10;
	}
#undef ABSMAX

	long long val = isnegative ? -((long long)(absval-1)) - 1 : (long long)absval;
	return integerobject_newfromlonglong(interp, errptr, val);
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
	// e.g. 0000000123 --> 123, 00000000 --> 0, 000001 --> 1
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
