#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "common.h"
#include "unicode.h"

int unicodestring_copyinto(struct UnicodeString src, struct UnicodeString *dst)
{
	uint32_t *val = malloc(sizeof(uint32_t) * src.len);
	if (!val)
		return STATUS_NOMEM;
	memcpy(val, src.val, sizeof(uint32_t) * src.len);
	dst->len = src.len;
	dst->val = val;
	return STATUS_OK;
}

struct UnicodeString *unicodestring_copy(struct UnicodeString src)
{
	struct UnicodeString *res = malloc(sizeof(struct UnicodeString));
	if (!res)
		return NULL;
	if (unicodestring_copyinto(src, res) == STATUS_NOMEM) {
		free(res);
		return NULL;
	}
	return res;
}

unsigned long unicodestring_hash(struct UnicodeString str)
{
	// djb2 hash
	// http://www.cse.yorku.ca/~oz/hash.html
	unsigned long hash = 5381;
	for (size_t i=0; i < str.len; i++)
		hash = hash*33 + str.val[i];
	return hash;
}


// example: ONES(6) is 111111 in binary
#define ONES(n) ((1<<(n))-1)


static inline int how_many_bytes(uint32_t codepnt, char *errormsg)
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
	// unsigned long is guaranteed to fit uint32_t
	sprintf(errormsg, "invalid Unicode code point U+%lX", (unsigned long) codepnt);
	return -1;
}


int utf8_encode(struct UnicodeString unicode, char **utf8, size_t *utf8len, char *errormsg)
{
	// don't set utf8len if this fails
	size_t utf8len_val = 0;
	int part;
	for (size_t i=0; i < unicode.len; i++) {
		if ((part = how_many_bytes(unicode.val[i], errormsg)) == -1) {
			// how_many_bytes() already set errormsg
			return 1;
		}
		utf8len_val += part;
	}

	unsigned char *ptr = malloc(utf8len_val);
	if (!ptr)
		return STATUS_NOMEM;

	// rest of this will not fail
	*utf8 = (char *) ptr;
	*utf8len = utf8len_val;

	for (size_t i=0; i < unicode.len; i++) {
		int nbytes = how_many_bytes(unicode.val[i], NULL);
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
			// get_utf8_length didn't work or how_many_bytes is inconsistent
			assert(0);
		}
		ptr += nbytes;
	}
	return STATUS_OK;
}


int utf8_decode(char *utf8, size_t utf8len, struct UnicodeString *unicode, char *errormsg)
{
	// must leave unicode and unicodelen untouched on error
	uint32_t *result;
	size_t resultlen = 0;

	// each utf8 byte is at most 1 unicode code point
	// this is realloc'd later to the correct size, feels better than many reallocs
	result = malloc(utf8len*sizeof(uint32_t));
	if (!result)
		return STATUS_NOMEM;

	unsigned char *u_utf8 = (unsigned char *) utf8;
	while (utf8len > 0) {
		uint32_t *ptr = result + resultlen;
		int nbytes;

#define CHECK_UTF8LEN()       do{ if (utf8len < (size_t)nbytes) { sprintf(errormsg, "unexpected end of string");           goto error; }}while(0)
#define CHECK_CONTINUATION(c) do{ if ((c)>>6 != 1<<1)           { sprintf(errormsg, "invalid continuation byte %#x", (c)); goto error; }}while(0)
		if (u_utf8[0] >> 7 == 0) {
			nbytes = 1;
			assert(!(utf8len < 1));   // otherwise the while loop shouldn't run...
			*ptr = u_utf8[0];
		}

		else if (u_utf8[0] >> 5 == ONES(2) << 1) {
			nbytes = 2;
			CHECK_UTF8LEN();
			CHECK_CONTINUATION(u_utf8[1]);
			*ptr = (ONES(5) & u_utf8[0])<<6 |
				(ONES(6) & u_utf8[1]);
		}

		else if (u_utf8[0] >> 4 == ONES(3) << 1) {
			nbytes = 3;
			CHECK_UTF8LEN();
			CHECK_CONTINUATION(u_utf8[1]);
			CHECK_CONTINUATION(u_utf8[2]);
			*ptr = ((uint32_t)(ONES(4) & u_utf8[0]))<<12UL |
				((uint32_t)(ONES(6) & u_utf8[1]))<<6UL |
				((uint32_t)(ONES(6) & u_utf8[2]));
		}

		else if (u_utf8[0] >> 3 == ONES(4) << 1) {
			nbytes = 4;
			CHECK_UTF8LEN();
			CHECK_CONTINUATION(u_utf8[1]);
			CHECK_CONTINUATION(u_utf8[2]);
			CHECK_CONTINUATION(u_utf8[3]);
			*ptr = ((uint32_t)(ONES(3) & u_utf8[0]))<<18UL |
				((uint32_t)(ONES(6) & u_utf8[1]))<<12UL |
				((uint32_t)(ONES(6) & u_utf8[2]))<<6UL |
				((uint32_t)(ONES(6) & u_utf8[3]));
		}
#undef CHECK_UTF8LEN
#undef CHECK_CONTINUATION

		else {
			sprintf(errormsg, "invalid start byte %#x", u_utf8[0]);
			goto error;
		}

		int expected_nbytes = how_many_bytes(*ptr, errormsg);
		if (expected_nbytes == -1) {
			// how_many_bytes has already set errormsg
			goto error;
		}
		assert(!(nbytes < expected_nbytes));
		if (nbytes > expected_nbytes) {
			// not worth making a utility function imo, too lazy to use switch-case
			if (nbytes == 2)
				sprintf(errormsg, "overlong encoding: %#x %#x", u_utf8[0], u_utf8[1]);
			else if (nbytes == 3)
				sprintf(errormsg, "overlong encoding: %#x %#x %#x", u_utf8[0], u_utf8[1], u_utf8[2]);
			else if (nbytes == 4)
				sprintf(errormsg, "overlong encoding: %#x %#x %#x %#x", u_utf8[0], u_utf8[1], u_utf8[2], u_utf8[3]);
			else
				assert(0);
			goto error;
		}

		resultlen += 1;
		u_utf8 += nbytes;
		utf8len -= nbytes;
	}

	// this realloc can't fail because it frees memory, never allocates more
	unicode->val = realloc(result, resultlen*sizeof(uint32_t));
	assert(resultlen ? !!unicode->val : 1);    // realloc(result, 0) may return NULL
	unicode->len = resultlen;
	return STATUS_OK;

error:
	if (result)
		free(result);
	return 1;
}
