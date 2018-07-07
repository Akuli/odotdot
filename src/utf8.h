#ifndef UTF8_H
#define UTF8_H

#include <stdbool.h>
#include <stddef.h>
#include "unicode.h"   // IWYU pragma: keep

// convert a Unicode string to a UTF-8 string
// after calling this, *utf8 may contain \0 bytes
// if you want a 0-terminated string, create a new string with 1 more byte at end set to 0
// returns false on error
bool utf8_encode(struct Interpreter *interp, struct UnicodeString unicode, char **utf8, size_t *utf8len);

// convert a UTF-8 string to a Unicode string
// if utf8 is \0-terminated, pass strlen(utf8) for utf8len
// returns false on error
bool utf8_decode(struct Interpreter *interp, char *utf8, size_t utf8len, struct UnicodeString *unicode);

#endif   // UTF8_H
