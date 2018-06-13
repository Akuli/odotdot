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

void test_integer_basic_stuff(void)
{
	buttert(strlen(INTEGEROBJECT_MINSTR) <= INTEGEROBJECT_MAXDIGITS);
	buttert(strlen(INTEGEROBJECT_MAXSTR) <= INTEGEROBJECT_MAXDIGITS);
}
