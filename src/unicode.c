#include "unicode.h"
#include <assert.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "common.h"   // i hate this file...........
#include "objects/errors.h"

/*
some stuff about error handling
objects/errors.c calls functions defined here
these functions set errors in the following cases:

1) running out of mem
	however, errors.c won't call these functions again
	the special nomemerr and its message string are created in builtins_setup()
	so they'll just be used then without creating new strings using these functions

2) invalid unicode or utf8
	the error messages will be ASCII only and never contain the invalid characters
	they may contain things like "U+ABCD", but not the actual character
*/

int unicodestring_copyinto(struct Interpreter *interp, struct UnicodeString src, struct UnicodeString *dst)
{
	unicode_char *val = malloc(sizeof(unicode_char) * src.len);
	if (!val) {
		errorobject_setnomem(interp);
		return STATUS_ERROR;
	}

	memcpy(val, src.val, sizeof(unicode_char) * src.len);
	dst->len = src.len;
	dst->val = val;
	return STATUS_OK;
}

struct UnicodeString *unicodestring_copy(struct Interpreter *interp, struct UnicodeString src)
{
	struct UnicodeString *res = malloc(sizeof(struct UnicodeString));
	if (!res) {
		errorobject_setnomem(interp);
		return NULL;
	}
	if (unicodestring_copyinto(interp, src, res) == STATUS_ERROR) {
		free(res);
		errorobject_setnomem(interp);
		return NULL;
	}
	return res;
}


static void error_printf(struct Interpreter *interp, char *fmt, ...)
{
	char msg[100];
	va_list ap;
	va_start(ap, fmt);
	vsnprintf(msg, 100, fmt, ap);
	va_end(ap);
	msg[99] = 0;
	errorobject_setwithfmt(interp, "%s", msg);
}


// reference for utf8 stuff: https://en.wikipedia.org/wiki/UTF-8

static int how_many_bytes(struct Interpreter *interp, unicode_char codepnt)
{
	if (codepnt <= 0x7f)
		return 1;
	if (codepnt <= 0x7ff)
		return 2;
	if (codepnt <= 0xffff) {
		if (0xd800 <= codepnt && codepnt <= 0xdfff)
			goto invalid_code_point;
		return 3;
	}
	if (codepnt <= 0x10ffff)
		return 4;

	// "fall through" to invalid_code_point

invalid_code_point:
	// unsigned long is at least 32 bits, so unicode_char should fit in it
	// errorobject_setwithfmt doesn't support %lX
	error_printf(interp, "invalid Unicode code point U+%lX", (unsigned long)codepnt);
	return -1;
}


// example: ONES(6) is 111111 in binary
#define ONES(n) ((1<<(n))-1)

int utf8_encode(struct Interpreter *interp, struct UnicodeString unicode, char **utf8, size_t *utf8len)
{
	// don't set utf8len if this fails
	size_t utf8len_val = 0;
	for (size_t i=0; i < unicode.len; i++) {
		int part = how_many_bytes(interp, unicode.val[i]);
		if (part == -1)
			return STATUS_ERROR;
		utf8len_val += part;
	}

	char *ptr = malloc(utf8len_val);
	if (!ptr) {
		errorobject_setnomem(interp);
		return STATUS_ERROR;
	}

	// rest of this will not fail
	*utf8 = ptr;
	*utf8len = utf8len_val;

	for (size_t i=0; i < unicode.len; i++) {
		int nbytes = how_many_bytes(interp, unicode.val[i]);
		switch (nbytes) {
		case 1:
			ptr[0] = unicode.val[i];
			break;
		case 2:
			ptr[0] = ONES(2)<<6 | unicode.val[i]>>6;
			ptr[1] = 1<<7 | (unicode.val[i] & ONES(6));
			break;
		case 3:
			ptr[0] = ONES(3)<<5 | unicode.val[i]>>12;
			ptr[1] = 1<<7 | (unicode.val[i]>>6 & ONES(6));
			ptr[2] = 1<<7 | (unicode.val[i] & ONES(6));
			break;
		case 4:
			ptr[0] = ONES(4)<<4 | unicode.val[i]>>18;
			ptr[1] = 1<<7 | (unicode.val[i]>>12 & ONES(6));
			ptr[2] = 1<<7 | (unicode.val[i]>>6 & ONES(6));
			ptr[3] = 1<<7 | (unicode.val[i] & ONES(6));
			break;
		default:
			// computing the length didn't work or how_many_bytes is inconsistent
			assert(0);
		}
		ptr += nbytes;
	}
	return STATUS_OK;
}


// there are many casts to unsigned char
// i don't want to cast a char pointer to unsigned char because i'm not sure if that's standardy
#define U(x) ((unsigned char)(x))

int utf8_decode(struct Interpreter *interp, char *utf8, size_t utf8len, struct UnicodeString *unicode)
{
	// must leave unicode and unicodelen untouched on error
	unicode_char *result;
	size_t resultlen = 0;

	// each utf8 byte is at most 1 unicode code point
	// this is realloc'd later to the correct size, feels better than many reallocs
	result = malloc(utf8len*sizeof(unicode_char));
	if (!result)
		return STATUS_NOMEM;

	while (utf8len > 0) {
		unicode_char *ptr = result + resultlen;
		int nbytes;

#define CHECK_UTF8LEN()       do{ if (utf8len < (size_t)nbytes) { error_printf(interp, "unexpected end of string");           goto error; }}while(0)
#define CHECK_CONTINUATION(c) do{ if ((c)>>6 != 1<<1)           { error_printf(interp, "invalid continuation byte %#x", (c)); goto error; }}while(0)
		if (U(utf8[0]) >> 7 == 0) {
			nbytes = 1;
			assert(!(utf8len < 1));   // otherwise the while loop shouldn't run...
			*ptr = U(utf8[0]);
		}

		else if (U(utf8[0]) >> 5 == ONES(2) << 1) {
			nbytes = 2;
			CHECK_UTF8LEN();
			CHECK_CONTINUATION(U(utf8[1]));
			*ptr = (ONES(5) & U(utf8[0]))<<6 |
				(ONES(6) & U(utf8[1]));
		}

		else if (U(utf8[0]) >> 4 == ONES(3) << 1) {
			nbytes = 3;
			CHECK_UTF8LEN();
			CHECK_CONTINUATION(U(utf8[1]));
			CHECK_CONTINUATION(U(utf8[2]));
			*ptr = ((unicode_char)(ONES(4) & U(utf8[0])))<<12UL |
				((unicode_char)(ONES(6) & U(utf8[1])))<<6UL |
				((unicode_char)(ONES(6) & U(utf8[2])));
		}

		else if (U(utf8[0]) >> 3 == ONES(4) << 1) {
			nbytes = 4;
			CHECK_UTF8LEN();
			CHECK_CONTINUATION(U(utf8[1]));
			CHECK_CONTINUATION(U(utf8[2]));
			CHECK_CONTINUATION(U(utf8[3]));
			*ptr = ((unicode_char)(ONES(3) & U(utf8[0])))<<18UL |
				((unicode_char)(ONES(6) & U(utf8[1])))<<12UL |
				((unicode_char)(ONES(6) & U(utf8[2])))<<6UL |
				((unicode_char)(ONES(6) & U(utf8[3])));
		}
#undef CHECK_UTF8LEN
#undef CHECK_CONTINUATION

		else {
			error_printf(interp, "invalid start byte %#x", (int) U(utf8[0]));
			goto error;
		}

		int expected_nbytes = how_many_bytes(interp, *ptr);
		if (expected_nbytes == -1) {
			// how_many_bytes has already set an error
			goto error;
		}
		assert(!(nbytes < expected_nbytes));
		if (nbytes > expected_nbytes) {
			if (nbytes == 2)
				error_printf(interp, "overlong encoding: %#x %#x", (int) U(utf8[0]), (int) U(utf8[1]));
			else if (nbytes == 3)
				error_printf(interp, "overlong encoding: %#x %#x %#x", (int) U(utf8[0]), (int) U(utf8[1]), (int) U(utf8[2]));
			else if (nbytes == 4)
				error_printf(interp, "overlong encoding: %#x %#x %#x %#x", (int) U(utf8[0]), (int) U(utf8[1]), (int) U(utf8[2]), (int) U(utf8[3]));
			else
				assert(0);
			goto error;
		}

		resultlen += 1;
		utf8 += nbytes;
		utf8len -= nbytes;
	}

	// this realloc can't fail because it frees memory, never allocates more
	unicode->val = realloc(result, resultlen*sizeof(unicode_char));
	assert(resultlen ? !!unicode->val : 1);    // realloc(result, 0) may return NULL
	unicode->len = resultlen;
	return STATUS_OK;

error:
	if (result)
		free(result);
	return STATUS_ERROR;
}

#undef U
