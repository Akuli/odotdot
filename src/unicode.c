#include "unicode.h"
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
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

bool unicodestring_copyinto(struct Interpreter *interp, struct UnicodeString src, struct UnicodeString *dst)
{
	if (src.len == 0) {     // malloc(0) may return NULL on success
		dst->len = 0;
		dst->val = NULL;
		return true;
	}

	unicode_char *val = malloc(sizeof(unicode_char) * src.len);
	if (!val) {
		errorobject_thrownomem(interp);
		return false;
	}

	memcpy(val, src.val, sizeof(unicode_char) * src.len);
	dst->len = src.len;
	dst->val = val;
	return true;
}

struct UnicodeString *unicodestring_copy(struct Interpreter *interp, struct UnicodeString src)
{
	struct UnicodeString *res = malloc(sizeof(struct UnicodeString));
	if (!res) {
		errorobject_thrownomem(interp);
		return NULL;
	}
	if (!unicodestring_copyinto(interp, src, res)) {
		free(res);
		return NULL;
	}
	return res;
}


// there are lots of corner cases... :D
struct UnicodeString *unicodestring_replace(struct Interpreter *interp, struct UnicodeString src, struct UnicodeString old, struct UnicodeString new)
{
	if (src.len == 0)   // malloc(0) may return NULL on success
		return unicodestring_copy(interp, src);

	if (old.len == 0) {
		// need to divide by old.len soon, and division by 0 is no good
		// also, what would .replace "" "lol" even do??
		errorobject_throwfmt(interp, "ValueError", "cannot replace \"\" with another string");
		return NULL;
	}
	if (src.len < old.len) {
		// src can't contain old
		return unicodestring_copy(interp, src);
	}

	struct UnicodeString *res = malloc(sizeof(struct UnicodeString));
	if (!res) {
		errorobject_thrownomem(interp);
		return NULL;
	}

	res->len = src.len;
	if (new.len > old.len) {
		// old fits in src floor(src.len/old.len) times, and C's size_t/size_t floor()s
		// so there may be at most src.len/old.len replacements
		// (max number of characters the string can grow) = (max number of replacements) * (length difference)
		res->val = malloc(sizeof(unicode_char) * (src.len + (src.len/old.len)*(new.len - old.len)));
	} else {
		// number of characters can't grow
		res->val = malloc(sizeof(unicode_char) * src.len);
	}
	if (!res->val) {
		free(res);
		errorobject_thrownomem(interp);
		return NULL;
	}
	memcpy(res->val, src.val, sizeof(unicode_char) * src.len);

	// now comes the replacing
	// replacing a substring with a different length messes up indexes AFTER the replacement
	// so must replace backwards
	size_t offset = src.len - old.len;    // can't be negative, see beginning of this function
	while (true) {
		// look for old in src at offset
		bool foundold = true;
		for (size_t i=0; i < old.len; i++) {
			if (src.val[offset + i] != old.val[i]) {
				foundold = false;
				break;
			}
		}
		if (!foundold) {
			if (offset == 0)
				return res;
			offset--;
			continue;
		}

		// this is the real thing... let's do the replace
		// first move stuff after offset+new.len out of the way, then add the new
		memmove(res->val+offset+new.len, res->val+offset+old.len, sizeof(unicode_char) * (res->len-offset-old.len));
		memcpy(res->val+offset, new.val, sizeof(unicode_char) * new.len);

		// i'm not sure what small_size_t - big_size_t does
		// probably something funny because size_t is unsigned but the result should be negative
		if (new.len > old.len)
			res->len += new.len - old.len;
		if (new.len < old.len)
			res->len -= old.len - new.len;

		// anything on the replaced area must not be replaced again
		// but avoid making the offset negative because it's unsigned
		if (offset < old.len)
			return res;
		offset -= old.len;
	}
}
