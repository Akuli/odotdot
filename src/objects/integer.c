// TODO: implement bignums
// TODO: better error handling than asserts

#include "integer.h"
#include <assert.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include "../common.h"
#include "../objectsystem.h"
#include "../unicode.h"

static int integer_destructor(struct Object *integer)
{
	free(integer->data);
	return STATUS_OK;
}

struct ObjectClassInfo *integerobject_createclass(struct ObjectClassInfo *objectclass)
{
	return objectclassinfo_new(objectclass, integer_destructor);
}

// TODO: better error handling for huge values
static struct Object *integer_from_digits(struct ObjectClassInfo *integerclass, int isnegative, int *digits, int ndigits)
{
	assert(ndigits > 0);

	// uint64_t can handle these because it doesn't use one bit for the sign
	uint64_t absval = 0;

	// careful here... -INT64_MIN doesn't fit in an int64_t, but -(INT64_MIN+1) fits
#define ABSMAX (uint64_t)(isnegative ? (uint64_t)(-(INT64_MIN + 1)) + 1 : INT64_MAX)

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

	struct Object *obj = object_new(integerclass);
	if (!obj) {
		free(data);
		return NULL;
	}
	obj->data = data;
	return obj;
}

struct Object *integerobject_newfromustr(struct ObjectClassInfo *integerclass, struct UnicodeString ustr)
{
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
	return integer_from_digits(integerclass, isnegative, digits, ustr.len);
}

struct Object *integerobject_newfromcharptr(struct ObjectClassInfo *integerclass, char *s)
{
	int isnegative=(s[0] == '-');
	if (isnegative)
		s++;

	size_t ndigits = strlen(s);
	assert(ndigits != 0);
	assert(ndigits < 25);

	// this is more than enough digits for 64-bit ints
	int digits[25];
	for (size_t i=0; i < ndigits; i++) {
		assert('0' <= s[i] && s[i] <= '9');
		digits[i] = s[i] - '0';
	}
	return integer_from_digits(integerclass, isnegative, digits, ndigits);
}

int64_t integerobject_toint64(struct Object *integer)
{
	return *((int64_t *) integer->data);
}
