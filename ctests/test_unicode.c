#include <src/attribute.h>
#include <src/interpreter.h>
#include <src/objects/string.h>
#include <src/objectsystem.h>
#include <src/operator.h>
#include <src/unicode.h>
#include <src/utf8.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include "utils.h"

void check(struct UnicodeString *s)
{
	buttert(!testinterp->err);
	buttert(s);
	buttert(s->len == 5);
	buttert(s->val[0] == 'h');
	buttert(s->val[1] == 'e');
	buttert(s->val[2] == 'l');
	buttert(s->val[3] == 'l');
	buttert(s->val[4] == 'o');
}

void test_copying(void)
{
	unicode_char avalues[] = { 'h', 'e', 'l', 'l', 'o' };
	struct UnicodeString a = { avalues, 5 };
	struct UnicodeString *b = unicodestring_copy(testinterp, a);
	check(b);
	free(b->val);
	free(b);

	struct UnicodeString *c = bmalloc(sizeof(struct UnicodeString));
	buttert(unicodestring_copyinto(testinterp, a, c) == true);
	check(c);
	free(c->val);
	free(c);
}
