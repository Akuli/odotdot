#include <stddef.h>

/* Convert a Unicode string to a UTF-8 string.

Return values:

- `0`: success
- `-1`: not enough memory
- `1`: the unicode string is invalid, an error message has been saved to `errormsg`

The error message is never more than 100 chars long (including the `\0`), so you
can pass an array of 100 chars to `errormsg`.
*/
int utf8_encode(unsigned long *unicode, size_t unicodelen, char **utf8, size_t *utf8len, char *errormsg);

/* Convert a UTF-8 string to a Unicode string.

Errors are handled similarly to [utf8_encode].
*/
int utf8_decode(char *utf8, size_t utf8len, unsigned long **unicode, size_t *unicodelen, char *errormsg);


// FIXME: unicode_isalpha sucks, maybe unicode_isdigit sucks too?
#define unicode_isalpha(x) ( \
	((unsigned long)'a' <= (x) && (x) <= (unsigned long)'z') || \
	((unsigned long)'A' <= (x) && (x) <= (unsigned long)'Z') || \
	(x) == 0xc5 /* Å */ || (x) == 0xc4 /* Ä */ || (x) == 0xd6 /* Ö */ || \
	(x) == 0xe5 /* å */ || (x) == 0xe4 /* ä */ || (x) == 0xf6 /* ö */ )
#define unicode_isdigit(x) ((unsigned long)'0' <= (x) && (x) <= (unsigned long)'9')
#define unicode_is0to9(x) ((unsigned long)'0' <= (x) && (x) <= (unsigned long)'9')
#define unicode_isalnum(x) (unicode_isalpha(x) || unicode_isdigit(x))
