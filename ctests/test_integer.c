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

static struct Object *cstring2integerobj(char *cstr)
{
	struct UnicodeString u;
	u.len = strlen(cstr);
	u.val = bmalloc(u.len * sizeof(unicode_char));
	for (size_t i=0; i<u.len; i++)
		u.val[i]=*cstr++;

	struct Object *asd = integerobject_newfromustr(testinterp, u);
	buttert(asd);
	return asd;
}

void test_integer_basic_stuff(void)
{
	buttert(strlen(INTEGEROBJECT_MINSTR) <= INTEGEROBJECT_MAXDIGITS);
	buttert(strlen(INTEGEROBJECT_MAXSTR) <= INTEGEROBJECT_MAXDIGITS);
}
