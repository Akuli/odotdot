// TODO: rename the utf8_{en,de}code functions to unicode_utf8{en,de}code?

#ifndef UNICODE_H
#define UNICODE_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

struct Interpreter;

// the standard says that int32_t is optional, but this must exist
// you can also use unsigned long when working with unicode chars, that's also >= 32 bits
// on many 64-bit systems, uint_least32_t is 32 bits but unsigned long is 64 bits
// so strings fit in smaller space with uint_least32_t
typedef uint_least32_t unicode_char;

struct UnicodeString {
	unicode_char *val;    // usually from malloc(), must be free()'d
	size_t len;
};

// the return value's ->val and the return value must be free()'d
// sets an error and returns NULL on failure
struct UnicodeString *unicodestring_copy(struct Interpreter *interp, struct UnicodeString src);

// dst->val must be free()'d
// returns false on error
bool unicodestring_copyinto(struct Interpreter *interp, struct UnicodeString src, struct UnicodeString *dst);

// replace all occurrences of old with new in src
// the return value's ->val and the return value must be free()'d
struct UnicodeString *unicodestring_replace(struct Interpreter *interp, struct UnicodeString src, struct UnicodeString old, struct UnicodeString new);


// convert a Unicode string to a UTF-8 string
// after calling this, *utf8 may contain \0 bytes
// if you want a 0-terminated string, create a new string with 1 more byte at end set to 0
// returns false on error
bool utf8_encode(struct Interpreter *interp, struct UnicodeString unicode, char **utf8, size_t *utf8len);

// convert a UTF-8 string to a Unicode string
// if utf8 is \0-terminated, pass strlen(utf8) for utf8len
// returns false on error
bool utf8_decode(struct Interpreter *interp, char *utf8, size_t utf8len, struct UnicodeString *unicode);


/* i cheated with python
	>>> for codepnt in range(0x110000):
	...     if chr(codepnt).isspace():
	...             print('(x) == %#x' % codepnt, end=' || ')
*/
#define unicode_isspace(x) ((x) == 0x9 || (x) == 0xa || (x) == 0xb || (x) == 0xc || (x) == 0xd || \
	(x) == 0x1c || (x) == 0x1d || (x) == 0x1e || (x) == 0x1f || (x) == 0x20 || (x) == 0x85 || \
	(x) == 0xa0 || (x) == 0x1680 || (x) == 0x2000 || (x) == 0x2001 || (x) == 0x2002 || \
	(x) == 0x2003 || (x) == 0x2004 || (x) == 0x2005 || (x) == 0x2006 || (x) == 0x2007 || \
	(x) == 0x2008 || (x) == 0x2009 || (x) == 0x200a || (x) == 0x2028 || (x) == 0x2029 || \
	(x) == 0x202f || (x) == 0x205f || (x) == 0x3000)

// TODO: make this suck less
#define unicode_isalpha(x) ( \
	('a' <= (x) && (x) <= 'z') || ('A' <= (x) && (x) <= 'Z') || \
	(x) == 0xc5 /* Å */ || (x) == 0xc4 /* Ä */ || (x) == 0xd6 /* Ö */ || \
	(x) == 0xe5 /* å */ || (x) == 0xe4 /* ä */ || (x) == 0xf6 /* ö */ )

#define unicode_isidentifier1st(x) ((x)=='_' || unicode_isalpha(x))
#define unicode_isidentifiernot1st(x) ((x)=='_' || unicode_isalpha(x) || ('0' <= (x) && (x) <= '9'))

#endif   // UNICODE_H
