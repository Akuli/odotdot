#include <src/objectsystem.h>     // IWYU pragma: keep
#include <src/objects/integer.h>
#include <src/unicode.h>
#include <stdint.h>
#include <string.h>
#include "utils.h"

void test_integer_basic_stuff(void)
{
	buttert(strlen(INTEGEROBJECT_MINSTR) <= INTEGEROBJECT_MAXDIGITS);
	buttert(strlen(INTEGEROBJECT_MAXSTR) <= INTEGEROBJECT_MAXDIGITS);
}
