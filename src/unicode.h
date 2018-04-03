// TODO: rename the utf8_{en,de}code functions to unicode_utf8{en,de}code?

#ifndef UNICODE_H
#define UNICODE_H

#include <stddef.h>
#include <stdint.h>

struct UnicodeString {
	uint32_t *val;    // usually from malloc(), must be free()'d
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


// these operate on uint32_t unicode characters, not UnicodeString structs
// FIXME: unicode_isalpha sucks, maybe unicode_isdigit sucks too?
#define unicode_isalpha(x) ( \
	('a' <= (x) && (x) <= 'z') || ('A' <= (x) && (x) <= 'Z') || \
	(x) == 0xc5 /* Å */ || (x) == 0xc4 /* Ä */ || (x) == 0xd6 /* Ö */ || \
	(x) == 0xe5 /* å */ || (x) == 0xe4 /* ä */ || (x) == 0xf6 /* ö */ )
#define unicode_isdigit(x) ('0' <= (x) && (x) <= '9')
#define unicode_isalnum(x) (unicode_isalpha(x) || unicode_isdigit(x))
#define unicode_isidentifier1st(x) (unicode_isalpha(x) || (x)=='_')
#define unicode_isidentifiernot1st(x) (unicode_isalnum(x) || (x)=='_')
#define unicode_is0to9(x) ('0' <= (x) && (x) <= '9')

#endif   // UNICODE_H
