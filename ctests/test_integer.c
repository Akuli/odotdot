#include <src/common.h>
#include <src/objectsystem.h>     // IWYU pragma: keep
#include <src/objects/integer.h>
#include <src/unicode.h>
#include <stdint.h>
#include <string.h>
#include "utils.h"

#if defined(INT64_MIN) && (INT64_MIN != INTEGEROBJECT_MIN)
#error INTEGEROBJECT_MIN is broken
#endif

#if defined(INT64_MAX) && (INT64_MAX != INTEGEROBJECT_MAX)
#error INTEGEROBJECT_MAX is broken
#endif

#define MANY_ASDS "asdasdasdasdasdasdasdasdasdasdasdasdasdasdasdasdasdasdasdasdasdasdasdasdasdasdasdasdasdasdasdasdasd"
#define MANY_ZEROS "00000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000"
#define ONE_TOO_BIG "9223372036854775808"
#define ONE_TOO_SMALL "-9223372036854775809"

void test_integer_basic_stuff(void)
{
	buttert(strlen(INTEGEROBJECT_MINSTR) <= INTEGEROBJECT_MAXDIGITS);
	buttert(strlen(INTEGEROBJECT_MAXSTR) <= INTEGEROBJECT_MAXDIGITS);

	struct UnicodeString u;
	u.len = 5;
	u.val = bmalloc(sizeof(unicode_char) * 5);
	u.val[0] = '-';
	u.val[1] = '0';     // leading zeros don't matter
	u.val[2] = '1';
	u.val[3] = '2';
	u.val[4] = '3';

	struct Object *negints[] = {
		integerobject_newfromustr(testinterp, NULL, u),
		integerobject_newfromcharptr(testinterp, NULL, "-0123"),
		integerobject_newfromcharptr(testinterp, NULL, "-"MANY_ZEROS"123") };

	// skip the minus sign
	u.len--;
	u.val++;

	struct Object *posints[] = {
		integerobject_newfromustr(testinterp, NULL, u),
		integerobject_newfromcharptr(testinterp, NULL, "0123"),
		integerobject_newfromcharptr(testinterp, NULL, MANY_ZEROS"123"), };
	free(u.val - 1 /* undo the minus skip */);

	buttert(sizeof(posints) == sizeof(negints));

	for (size_t i=0; i < sizeof(posints)/sizeof(posints[0]); i++) {
		buttert(posints[i]);
		buttert(negints[i]);
		buttert(integerobject_tolonglong(posints[i]) == 123);
		buttert(integerobject_tolonglong(negints[i]) == -123);
		OBJECT_DECREF(testinterp, posints[i]);
		OBJECT_DECREF(testinterp, negints[i]);
	}

	// "0" and "-0" should be treated equally
	struct Object *zero1 = integerobject_newfromcharptr(testinterp, NULL, "0");
	struct Object *zero2 = integerobject_newfromcharptr(testinterp, NULL, "-0");
	buttert(integerobject_tolonglong(zero1) == 0);
	buttert(integerobject_tolonglong(zero2) == 0);
	OBJECT_DECREF(testinterp, zero1);
	OBJECT_DECREF(testinterp, zero2);
}

static void check_error(struct Object *err, char *msg)
{
	// FIXME: utf8_encode() and utf8_decode() suck
	buttert(err);
	struct UnicodeString *errustr = ((struct Object*) err->data)->data;
	char *errstr;
	size_t errstrlen;
	buttert(utf8_encode(*errustr, &errstr, &errstrlen, NULL) == STATUS_OK);
	buttert((errstr = realloc(errstr, errstrlen+1)));
	errstr[errstrlen] =0;
	buttert2(strcmp(errstr, msg) == 0, errstr);
	free(errstr);
}

void test_integer_huge_tiny(void)
{
	struct Object *tiny = integerobject_newfromcharptr(testinterp, NULL, "-9223372036854775808");
	struct Object *huge = integerobject_newfromcharptr(testinterp, NULL, "9223372036854775807");
	buttert(integerobject_tolonglong(tiny) == INTEGEROBJECT_MIN);
	buttert(integerobject_tolonglong(huge) == INTEGEROBJECT_MAX);
	OBJECT_DECREF(testinterp, tiny);
	OBJECT_DECREF(testinterp, huge);

	struct Object *err;
#define CHECK(x) \
		buttert(integerobject_newfromcharptr(testinterp, &err, x) == NULL); \
		check_error(err, "integers must be between -9223372036854775808 and 9223372036854775807, but '"x"' is not"); \
		OBJECT_DECREF(testinterp, err);
	CHECK(ONE_TOO_BIG);
	CHECK(ONE_TOO_SMALL);
#undef CHECK
}

void test_integer_errors(void)
{
	struct Object *err;
#define CHECK(x) \
		buttert(integerobject_newfromcharptr(testinterp, &err, x) == NULL); \
		check_error(err, "'"x"' is not an integer"); \
		OBJECT_DECREF(testinterp, err);
	CHECK("1+2");
	CHECK("0000-123");
	CHECK("0-123");
	CHECK("--123");
	CHECK("+123");   // TODO: should this be treated as 123?
	CHECK("++123");
	CHECK("+-123");
	CHECK("-+123");
	CHECK("asdasdasd");
	// FIXME: this gives the wrong error message
	//CHECK("asdasdasdasdasdasdasdasdasdasdasdasdasdasdasdasdasdasdasdasdasdasdasdasdasdasdasdasdasdasdasdasdasdasdasdasdasdasdasdasdasdasd");
#undef CHECK
}
