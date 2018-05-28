// TODO: rename the utf8_{en,de}code functions to unicode_utf8{en,de}code?

#ifndef UNICODE_H
#define UNICODE_H

#include <stddef.h>
#include <stdint.h>

// the standard says that int32_t is optional, but this must exist
typedef uint_least32_t unicode_char;

struct UnicodeString {
	unicode_char *val;    // usually from malloc(), must be free()'d
	size_t len;
};

// never fails
unsigned int unicodestring_hash(struct UnicodeString str);

// the return value's ->val and the return value must be free()'d
struct UnicodeString *unicodestring_copy(struct UnicodeString src);

// returns STATUS_OK or STATUS_NOMEM, dst->val must be free()'d
int unicodestring_copyinto(struct UnicodeString src, struct UnicodeString *dst);


/* Convert a Unicode string to a UTF-8 string.

The resulting utf8 may contain \0 bytes. If you want a \0-terminated string,
realloc the resulting utf8 to have 1 more byte and set that last byte to 0.

If the unicode string is invalid, an error message is saved to errormsg and
this returns 1. The error message is \0-terminated, and it will always fit in
an array of 100 chars.

If the unicode string is valid, this returns STATUS_OK or STATUS_NOMEM.
*/
int utf8_encode(struct UnicodeString unicode, char **utf8, size_t *utf8len, char *errormsg);

/* Convert a UTF-8 string to a Unicode string.

The utf8 may contain \0 bytes. If it's \0-terminated, pass strlen(utf8) to
utf8len.

Errors are handled similarly to utf8_encode.
*/
int utf8_decode(char *utf8, size_t utf8len, struct UnicodeString *unicode, char *errormsg);


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
