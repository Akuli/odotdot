/* most of the interesting stuff is implemented in ../unicode.c, this is just a wrapper

if you're looking for how escapes like \n and \t are implemented, you also came to the wrong place :D
check these places instead:
	* src/tokenizer.c makes sure that escapes in string literals are valid and returns the literals as-is
	* src/parse.c assumes that the literals are valid and parses them
	* std/builtins.ö implements String's to_debug_string

*/

#include "string.h"
#include <assert.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../attribute.h"
#include "../check.h"
#include "../interpreter.h"
#include "../method.h"
#include "../objectsystem.h"
#include "../unicode.h"
#include "../utf8.h"
#include "array.h"
#include "bool.h"
#include "classobject.h"
#include "errors.h"
#include "function.h"
#include "integer.h"
#include "null.h"

static void string_destructor(struct Object *s)
{
	struct UnicodeString *data = s->data;
	free(data->val);
	free(data);
}

static struct Object *newinstance(struct Interpreter *interp, struct Object *args, struct Object *opts)
{
	errorobject_throwfmt(interp, "TypeError", "strings can't be created with (new String), use \"text in quotes\" instead");
	return NULL;
}

struct Object *stringobject_createclass_noerr(struct Interpreter *interp)
{
	return classobject_new_noerr(interp, interp->builtins.Object, newinstance);
}


// returns the string itself, for consistency with other types
static struct Object *to_string(struct Interpreter *interp, struct Object *args, struct Object *opts)
{
	if (!check_args(interp, args, interp->builtins.String, NULL)) return NULL;
	if (!check_no_opts(interp, opts)) return NULL;

	OBJECT_INCREF(interp, ARRAYOBJECT_GET(args, 0));
	return ARRAYOBJECT_GET(args, 0);
}


static struct Object *length_getter(struct Interpreter *interp, struct Object *args, struct Object *opts)
{
	if (!check_args(interp, args, interp->builtins.String, NULL)) return NULL;
	if (!check_no_opts(interp, opts)) return NULL;

	return integerobject_newfromlonglong(interp, ((struct UnicodeString*) ARRAYOBJECT_GET(args, 0)->data)->len);
}

// forward decla
static struct Object *new_string_from_ustr_with_no_copy(struct Interpreter *, struct UnicodeString *);

static struct Object *replace(struct Interpreter *interp, struct Object *args, struct Object *opts)
{
	// TODO: allow passing in a Mapping of things to replace?
	if (!check_args(interp, args, interp->builtins.String, interp->builtins.String, interp->builtins.String, NULL)) return NULL;
	if (!check_no_opts(interp, opts)) return NULL;

	struct UnicodeString *src = ARRAYOBJECT_GET(args, 0)->data;
	struct UnicodeString *old = ARRAYOBJECT_GET(args, 1)->data;
	struct UnicodeString *new = ARRAYOBJECT_GET(args, 2)->data;

	struct UnicodeString *replaced = unicodestring_replace(interp, *src, *old, *new);
	if (!replaced)
		return NULL;

	struct Object *res = new_string_from_ustr_with_no_copy(interp, replaced);
	if (!res) {
		free(replaced->val);
		free(replaced);
		return NULL;
	}
	return res;
}

// get and slice are a lot like array methods
// some day strings will hopefully behave like an immutable array of 1-character strings
static struct Object *get(struct Interpreter *interp, struct Object *args, struct Object *opts)
{
	if (!check_args(interp, args, interp->builtins.String, interp->builtins.Integer, NULL)) return NULL;
	if (!check_no_opts(interp, opts)) return NULL;

	struct UnicodeString ustr = *((struct UnicodeString*) ARRAYOBJECT_GET(args, 0)->data);
	long long i = integerobject_tolonglong(ARRAYOBJECT_GET(args, 1));

	if (i < 0) {
		errorobject_throwfmt(interp, "ValueError", "%L is not a valid string index", i);
		return NULL;
	}
	if ((unsigned long long) i >= ustr.len) {
		errorobject_throwfmt(interp, "ValueError", "%L is not a valid index for a string of length %L", i, (long long) ustr.len);
		return NULL;
	}

	ustr.val += i;
	ustr.len = 1;
	return stringobject_newfromustr(interp, ustr);
}

static struct Object *slice(struct Interpreter *interp, struct Object *args, struct Object *opts)
{
	if (!check_no_opts(interp, opts))
		return NULL;

	long long start, end;
	if (ARRAYOBJECT_LEN(args) == 2) {
		// (s.slice start)
		if (!check_args(interp, args, interp->builtins.String, interp->builtins.Integer, NULL))
			return NULL;
		start = integerobject_tolonglong(ARRAYOBJECT_GET(args, 1));
		end = ((struct UnicodeString*) ARRAYOBJECT_GET(args, 0)->data)->len;
	} else {
		// (s.slice start end)
		if (!check_args(interp, args, interp->builtins.String, interp->builtins.Integer, interp->builtins.Integer, NULL))
			return NULL;
		start = integerobject_tolonglong(ARRAYOBJECT_GET(args, 1));
		end = integerobject_tolonglong(ARRAYOBJECT_GET(args, 2));
	}

	struct UnicodeString ustr = *((struct UnicodeString*) ARRAYOBJECT_GET(args, 0)->data);

	if (start < 0)
		start = 0;
	if (end < 0)
		return stringobject_newfromcharptr(interp, "");
	// now end can be casted to size_t
	if ((size_t) end > ustr.len)
		end = ustr.len;
	if (start >= end)
		return stringobject_newfromcharptr(interp, "");

	if (start == 0 && (size_t) end == ustr.len) {
		struct Object *s = ARRAYOBJECT_GET(args, 0);
		OBJECT_INCREF(interp, s);
		return s;
	}

	ustr.val += start;
	ustr.len = end - start;
	return stringobject_newfromustr(interp, ustr);
}

struct Object *stringobject_splitbywhitespace(struct Interpreter *interp, struct Object *s)
{
	size_t offset = 0;
	while (offset < STRINGOBJECT_LEN(s) && unicode_isspace(STRINGOBJECT_GET(s, offset)))
		offset++;

	struct Object *result = arrayobject_newempty(interp);
	if (!result)
		return NULL;

	while (offset < STRINGOBJECT_LEN(s)) {
		bool found_ws = false;

		assert(!unicode_isspace(STRINGOBJECT_GET(s, offset)));
		for (size_t nows_end = offset+1; nows_end < STRINGOBJECT_LEN(s); nows_end++) {
			// skip characters until whitespace is found
			if (!unicode_isspace(STRINGOBJECT_GET(s, nows_end)))
				continue;

			// slice it and push the slice to result
			struct UnicodeString subu = *((struct UnicodeString*) s->data);
			subu.val += offset;
			subu.len = nows_end - offset;
			struct Object *sub = stringobject_newfromustr(interp, subu);
			if (!sub)
				goto error;
			bool ok = arrayobject_push(interp, result, sub);
			OBJECT_DECREF(interp, sub);
			if (!ok)
				goto error;

			// skip 1 or more whitespaces
			offset = nows_end+1;
			while (offset < STRINGOBJECT_LEN(s) && unicode_isspace(STRINGOBJECT_GET(s, offset)))
				offset++;

			found_ws = true;
			break;      // breaks the for loop
		}

		if (!found_ws) {
			// rest of the string is non-whitespace
			struct UnicodeString lastu = *((struct UnicodeString*) s->data);
			lastu.val += offset;
			lastu.len -= offset;
			struct Object *last = stringobject_newfromustr(interp, lastu);
			if (!last)
				goto error;
			bool ok = arrayobject_push(interp, result, last);
			OBJECT_DECREF(interp, last);
			if (!ok)
				goto error;
			break;    // breaks the while loop
		}
	}

	// no need to incref, this thing is already holding a reference to the result
	return result;

error:
	OBJECT_DECREF(interp, result);
	return NULL;
}

static struct Object *split_by_whitespace(struct Interpreter *interp, struct Object *args, struct Object *opts)
{
	if (!check_args(interp, args, interp->builtins.String, NULL)) return NULL;
	if (!check_no_opts(interp, opts)) return NULL;
	return stringobject_splitbywhitespace(interp, ARRAYOBJECT_GET(args, 0));
}

bool stringobject_addmethods(struct Interpreter *interp)
{
	// TODO: create many more string methods
	if (!attribute_add(interp, interp->builtins.String, "length", length_getter, NULL)) return false;
	if (!method_add(interp, interp->builtins.String, "get", get)) return false;
	if (!method_add(interp, interp->builtins.String, "replace", replace)) return false;
	if (!method_add(interp, interp->builtins.String, "slice", slice)) return false;
	if (!method_add(interp, interp->builtins.String, "split_by_whitespace", split_by_whitespace)) return false;
	if (!method_add(interp, interp->builtins.String, "to_string", to_string)) return false;
	return true;
}


static struct Object *eq(struct Interpreter *interp, struct Object *args, struct Object *opts)
{
	if (!check_args(interp, args, interp->builtins.Object, interp->builtins.Object, NULL)) return NULL;
	if (!check_no_opts(interp, opts)) return NULL;

	struct Object *s1 = ARRAYOBJECT_GET(args, 0), *s2 = ARRAYOBJECT_GET(args, 1);
	if (!(classobject_isinstanceof(s1, interp->builtins.String) && classobject_isinstanceof(s2, interp->builtins.String)))
		return nullobject_get(interp);

	struct UnicodeString *u1 = s1->data;
	struct UnicodeString *u2 = s2->data;
	if (u1->len != u2->len)
		return boolobject_get(interp, false);

	// memcmp is not reliable :( https://stackoverflow.com/a/11995514
	// TODO: use memcmp on systems where it works reliably (i have an idea for checking it)
	for (size_t i=0; i < u1->len; i++) {
		if (u1->val[i] != u2->val[i])
			return boolobject_get(interp, false);
	}
	return boolobject_get(interp, true);
}

// concatenates strings
static struct Object *add(struct Interpreter *interp, struct Object *args, struct Object *opts)
{
	if (!check_args(interp, args, interp->builtins.Object, interp->builtins.Object, NULL)) return NULL;
	if (!check_no_opts(interp, opts)) return NULL;

	struct Object *s1 = ARRAYOBJECT_GET(args, 0);
	struct Object *s2 = ARRAYOBJECT_GET(args, 1);
	if (!(classobject_isinstanceof(s1, interp->builtins.String) && classobject_isinstanceof(s2, interp->builtins.String)))
		return nullobject_get(interp);

	struct UnicodeString u1 = *((struct UnicodeString*) ARRAYOBJECT_GET(args, 0)->data);
	struct UnicodeString u2 = *((struct UnicodeString*) ARRAYOBJECT_GET(args, 1)->data);

	struct UnicodeString *u = malloc(sizeof(struct UnicodeString));
	if (!u) {
		errorobject_thrownomem(interp);
		return NULL;
	}

	u->len = u1.len + u2.len;
	u->val = malloc(sizeof(unicode_char) * u->len);
	if (!u->val) {
		errorobject_thrownomem(interp);
		free(u);
		return NULL;
	}

	memcpy(u->val, u1.val, u1.len * sizeof(unicode_char));
	memcpy(u->val + u1.len, u2.val, u2.len * sizeof(unicode_char));
	return new_string_from_ustr_with_no_copy(interp, u);
}

bool stringobject_initoparrays(struct Interpreter *interp) {
	if (!functionobject_add2array(interp, interp->oparrays.eq, "string_eq", eq)) return false;
	if (!functionobject_add2array(interp, interp->oparrays.add, "string_add", add)) return false;
	return true;
}


static long string_hash(struct UnicodeString u)
{
	// djb2 hash
	// http://www.cse.yorku.ca/~oz/hash.html
	unsigned long hash = 5381;
	for (size_t i=0; i < u.len; i++)
		hash = hash*33 + u.val[i];
	return (long)hash;
}

// TODO: expose this and use it everywhere
static struct Object *new_string_from_ustr_with_no_copy(struct Interpreter *interp, struct UnicodeString *ustr)
{
	struct Object *s = object_new_noerr(interp, interp->builtins.String, ustr, NULL, string_destructor);
	if (!s) {
		errorobject_thrownomem(interp);
		return NULL;
	}
	s->hash = string_hash(*ustr);
	return s;
}

struct Object *stringobject_newfromustr(struct Interpreter *interp, struct UnicodeString ustr)
{
	struct UnicodeString *data = unicodestring_copy(interp, ustr);
	if (!data)
		return NULL;

	struct Object *res = new_string_from_ustr_with_no_copy(interp, data);
	if (!res) {
		free(data->val);
		free(data);
		return NULL;
	}
	return res;
}

struct Object *stringobject_newfromcharptr(struct Interpreter *interp, char *ptr)
{
	struct UnicodeString *data = malloc(sizeof(struct UnicodeString));
	if (!data)
		return NULL;

	if (!utf8_decode(interp, ptr, strlen(ptr), data)) {
		free(data);
		return NULL;
	}

	struct Object *res = new_string_from_ustr_with_no_copy(interp, data);
	if (!res) {
		free(data->val);
		free(data);
		return NULL;
	}
	return res;
}


#define POINTER_MAXSTR 50            // should be big enough
#define MAX_PARTS 20                 // feel free to make this bigger
#define BETWEEN_SPECIFIERS_MAX 200   // makes really long error messages possible... not sure if that's good
struct Object *stringobject_newfromvfmt(struct Interpreter *interp, char *fmt, va_list ap)
{
	struct UnicodeString parts[MAX_PARTS];
	int nparts = 0;

	// setting everything to 0 is much easier than setting to 1
	bool skipfreeval[MAX_PARTS] = {0};    // set skipfreeval[i]=true to prevent freeing parts[i].val

	// there are at most MAX_PARTS of these
	// can't decref right away if the data of the object is still used in this function
	struct Object *gonnadecref[MAX_PARTS+1 /* always ends with NULL */] = { NULL };
	struct Object **gonnadecrefptr = gonnadecref;   // add a new object like *gonnadecrefptr++ = ...

	while(*fmt) {
		if (*fmt == '%') {
			fmt += 2;  // skip % followed by some character

			if (*(fmt-1) == 'p') {   // a pointer
				void *ptr = va_arg(ap, void*);

				char msg[POINTER_MAXSTR+1];
				snprintf(msg, POINTER_MAXSTR, "%p", ptr);
				msg[POINTER_MAXSTR] = 0;
				assert(msg[0] != 0);    // must not be empty because malloc(0) may return NULL on success

				parts[nparts].len = strlen(msg);
				parts[nparts].val = malloc(sizeof(unicode_char) * parts[nparts].len);
				if (!parts[nparts].val)
					goto nomem;

				// decodes with ASCII
				// can't memcpy because different types
				for (int i=0; i < (int) parts[nparts].len; i++)
					parts[nparts].val[i] = msg[i];
			}

			else if (*(fmt-1) == 's') {   // c char pointer
				char *part = va_arg(ap, char*);
				if (!utf8_decode(interp, part, strlen(part), parts + nparts))
					goto error;
			}

			else if (*(fmt-1) == 'U') {   // struct UnicodeString
				parts[nparts] = va_arg(ap, struct UnicodeString);
				skipfreeval[nparts] = true;
			}

			else if (*(fmt-1) == 'S' || *(fmt-1) == 'D') {   // to_string or to_debug_string
				// no need to mess with refcounts, let's consistently not touch them
				struct Object *obj = va_arg(ap, struct Object *);
				assert(obj);

				struct Object *strobj;
				if (*(fmt-1) == 'D')
					strobj = method_call_todebugstring(interp, obj);
				else
					strobj = method_call_tostring(interp, obj);
				if (!strobj)
					goto error;
				*gonnadecrefptr++ = strobj;

				parts[nparts] = *((struct UnicodeString *) strobj->data);
				skipfreeval[nparts] = true;
			}

			else if (*(fmt-1) == 'L') {   // long long
				long long val = va_arg(ap, long long);

				// TODO: don't copy/paste from above, move the stringifying code here instead?
				struct Object *intobj = integerobject_newfromlonglong(interp, val);
				if (!intobj)
					goto error;
				struct Object *strobj = method_call_tostring(interp, intobj);
				OBJECT_DECREF(interp, intobj);
				if (!strobj)
					goto error;
				*gonnadecrefptr++ = strobj;

				parts[nparts] = *((struct UnicodeString *) strobj->data);
				skipfreeval[nparts] = true;
			}

			else if (*(fmt-1) == '%') {   // literal %
				parts[nparts].len = 1;
				parts[nparts].val = malloc(sizeof(unicode_char));
				if (!(parts[nparts].val))
					goto nomem;
				*(parts[nparts].val) = '%';
			}

			else {
				assert(0);   // unknown format character
			}

		} else {
			char part[BETWEEN_SPECIFIERS_MAX];
			int len = 0;
			while (*fmt != '%' && *fmt)
				part[len++] = *fmt++;
			if (!utf8_decode(interp, part, len, parts + nparts))
				goto error;
		}

		nparts++;
	}

	size_t totallen = 0;
	for (int i=0; i < nparts; i++)
		totallen += parts[i].len;

	struct Object *res;
	if (totallen == 0) {    // malloc(0) may return NULL on success, avoid that
		if (!(res = stringobject_newfromcharptr(interp, "")))
			goto error;
	} else {
		struct UnicodeString *ures = malloc(sizeof(struct UnicodeString));
		if (!ures)
			goto nomem;

		ures->len = totallen;
		ures->val = malloc(sizeof(unicode_char) * totallen);
		if (!ures->val) {
			free(ures);
			goto nomem;
		}

		unicode_char *ptr = ures->val;
		for (int i=0; i < nparts; i++) {
			memcpy(ptr, parts[i].val, sizeof(unicode_char) * parts[i].len);
			ptr += parts[i].len;
		}

		if (!(res = new_string_from_ustr_with_no_copy(interp, ures))) {
			free(ures->val);
			free(ures);
			goto error;
		}
	}

	assert(res);
	for (int i=0; i < nparts; i++) {
		if (!skipfreeval[i])
			free(parts[i].val);
	}
	for (int i=0; gonnadecref[i]; i++)
		OBJECT_DECREF(interp, gonnadecref[i]);
	return res;

nomem:
	errorobject_thrownomem(interp);
	// "fall through" to error

error:
	for (int i=0; i < nparts; i++) {
		if (!skipfreeval[i])
			free(parts[i].val);
	}
	for (int i=0; gonnadecref[i]; i++)
		OBJECT_DECREF(interp, gonnadecref[i]);
	return NULL;
}

struct Object *stringobject_newfromfmt(struct Interpreter *interp, char *fmt, ...)
{
	va_list ap;
	va_start(ap, fmt);
	struct Object *res = stringobject_newfromvfmt(interp, fmt, ap);
	va_end(ap);
	return res;
}
