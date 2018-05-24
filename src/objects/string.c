// most of the interesting stuff is implemented in ../unicode.c, this is just a wrapper

#include "string.h"
#include <assert.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "classobject.h"
#include "errors.h"
#include "function.h"
#include "integer.h"
#include "../common.h"
#include "../interpreter.h"
#include "../method.h"
#include "../objectsystem.h"
#include "../unicode.h"

static void string_destructor(struct Object *str)
{
	struct UnicodeString *data = str->data;
	free(data->val);
	free(data);
}

struct Object *stringobject_createclass_noerr(struct Interpreter *interp)
{
	return classobject_new_noerr(interp, "String", interp->builtins.objectclass, 0, NULL);
}

static struct Object *to_string(struct Interpreter *interp, struct Object **args, size_t nargs)
{
	if (functionobject_checktypes(interp, args, nargs, interp->builtins.stringclass, NULL) == STATUS_ERROR)
		return NULL;

	OBJECT_INCREF(interp, args[0]);   // we're returning a reference
	return args[0];
}

static struct Object *to_debug_string(struct Interpreter *interp, struct Object **args, size_t nargs)
{
	if (functionobject_checktypes(interp, args, nargs, interp->builtins.stringclass, NULL) == STATUS_ERROR)
		return NULL;

	struct UnicodeString noquotes = *((struct UnicodeString*) args[0]->data);
	struct UnicodeString yesquotes;
	yesquotes.len = noquotes.len + 2;
	yesquotes.val = malloc(sizeof(unicode_char) * yesquotes.len);
	if (!yesquotes.val) {
		errorobject_setnomem(interp);
		return NULL;
	}
	yesquotes.val[0] = yesquotes.val[yesquotes.len - 1] = '"';
	memcpy(yesquotes.val + 1, noquotes.val, sizeof(unicode_char) * noquotes.len);

	struct Object *res = stringobject_newfromustr(interp, yesquotes);
	free(yesquotes.val);
	return res;
}

int stringobject_addmethods(struct Interpreter *interp)
{
	// TODO: create many more string methods
	if (method_add(interp, interp->builtins.stringclass, "to_string", to_string) == STATUS_ERROR) return STATUS_ERROR;
	if (method_add(interp, interp->builtins.stringclass, "to_debug_string", to_debug_string) == STATUS_ERROR) return STATUS_ERROR;
	return STATUS_OK;
}

struct Object *stringobject_newfromustr(struct Interpreter *interp, struct UnicodeString ustr)
{
	struct UnicodeString *data = unicodestring_copy(ustr);
	if (!data) {
		errorobject_setnomem(interp);
		return NULL;
	}

	struct Object *str = classobject_newinstance(interp, interp->builtins.stringclass, data, string_destructor);
	if (!str) {
		free(data->val);
		free(data);
		return NULL;
	}
	str->hash = unicodestring_hash(ustr);
	return str;
}

struct Object *stringobject_newfromcharptr(struct Interpreter *interp, char *ptr)
{
	struct UnicodeString *data = malloc(sizeof(struct UnicodeString));
	if (!data)
		return NULL;

	char errormsg[100];
	int status = utf8_decode(ptr, strlen(ptr), data, errormsg);
	assert(status != 1);   // must be valid UTF8
	if (status == STATUS_NOMEM) {
		free(data);
		return NULL;
	}
	assert(status == STATUS_OK);   // it shooouldn't return anything else than STATUS_{NONEM,OK} or 1

	struct Object *str = classobject_newinstance(interp, interp->builtins.stringclass, data, string_destructor);
	if (!str) {
		free(data->val);
		free(data);
		return NULL;
	}
	str->hash = unicodestring_hash(*data);
	return str;
}

#define POINTER_MAXSTR 50            // should be big enough
#define MAX_PARTS 20                 // feel free to make this bigger
#define BETWEEN_SPECIFIERS_MAX 200   // makes really long error messages possible... not sure if that's good
struct Object *stringobject_newfromvfmt(struct Interpreter *interp, char *fmt, va_list ap)
{
	struct UnicodeString parts[MAX_PARTS];
	int nparts = 0;

	// setting everything to 0 is much easier than setting to 1
	int skipfreeval[MAX_PARTS] = {0};    // set skipfreeval[i]=1 to prevent freeing parts[i].val

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

				parts[nparts].len = strlen(msg);
				parts[nparts].val = malloc(sizeof(unicode_char) * parts[nparts].len);
				if (!parts[nparts].val)
					goto nomem;

				// can't memcpy because different types
				for (int i=0; i < (int) parts[nparts].len; i++)
					parts[nparts].val[i] = msg[i];
			}

			else if (*(fmt-1) == 's') {   // c char pointer
				char *part = va_arg(ap, char*);

				// segfaults if the part is not valid utf8 because the NULL
				if (utf8_decode(part, strlen(part), parts + nparts, NULL) != STATUS_OK)
					goto nomem;
			}

			else if (*(fmt-1) == 'U') {   // struct UnicodeString
				parts[nparts] = va_arg(ap, struct UnicodeString);
				skipfreeval[nparts] = 1;
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
				skipfreeval[nparts] = 1;
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
				skipfreeval[nparts] = 1;
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
			if (utf8_decode(part, len, parts + nparts, NULL) != STATUS_OK)
				goto nomem;
		}

		nparts++;
	}

	struct UnicodeString everything;
	everything.len = 0;
	for (int i=0; i < nparts; i++)
		everything.len += parts[i].len;

	everything.val = malloc(sizeof(unicode_char) * everything.len);
	if (!everything.val)
		goto nomem;

	unicode_char *ptr = everything.val;
	for (int i=0; i < nparts; i++) {
		memcpy(ptr, parts[i].val, sizeof(unicode_char) * parts[i].len);
		ptr += parts[i].len;
	}

	struct Object *res = stringobject_newfromustr(interp, everything);

	free(everything.val);
	for (int i=0; i < nparts; i++) {
		if (!skipfreeval[i])
			free(parts[i].val);
	}
	for (int i=0; gonnadecref[i]; i++)
		OBJECT_DECREF(interp, gonnadecref[i]);

	return res;

nomem:
	errorobject_setnomem(interp);
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
