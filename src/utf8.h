#ifndef UTF8_H
#define UTF8_H

#include <stddef.h>
#include <stdint.h>

typedef uint32_t unicode_t;

/* Convert a Unicode string to a UTF-8 string.

If the unicode string is invalid, an error message is saved to errormsg and
this returns 1. Otherwise this returns STATUS_OK or STATUS_NOMEM.
*/
int utf8_encode(unicode_t *unicode, size_t unicodelen, char **utf8, size_t *utf8len, char *errormsg);

/* Convert a UTF-8 string to a Unicode string.

Errors are handled similarly to utf8_encode.
*/
int utf8_decode(char *utf8, size_t utf8len, unicode_t **unicode, size_t *unicodelen, char *errormsg);


// FIXME: unicode_isalpha sucks, maybe unicode_isdigit sucks too?
#define unicode_isalpha(x) ( \
	((unicode_t)'a' <= (x) && (x) <= (unicode_t)'z') || \
	((unicode_t)'A' <= (x) && (x) <= (unicode_t)'Z') || \
	(x) == 0xc5 /* Å */ || (x) == 0xc4 /* Ä */ || (x) == 0xd6 /* Ö */ || \
	(x) == 0xe5 /* å */ || (x) == 0xe4 /* ä */ || (x) == 0xf6 /* ö */ )
#define unicode_isdigit(x) ((unicode_t)'0' <= (x) && (x) <= (unicode_t)'9')
#define unicode_is0to9(x) ((unicode_t)'0' <= (x) && (x) <= (unicode_t)'9')
#define unicode_isalnum(x) (unicode_isalpha(x) || unicode_isdigit(x))

#endif   // UTF8_H
