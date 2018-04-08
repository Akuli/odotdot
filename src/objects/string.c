// most of the interesting stuff is implemented in ../unicode.c, this is just a wrapper

#include "string.h"
#include <assert.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include "classobject.h"
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

struct ObjectClassInfo *stringobject_createclass(struct ObjectClassInfo *objectclass)
{
	return objectclassinfo_new("String", objectclass, NULL, string_destructor);
}

static struct Object *to_string(struct Context *ctx, struct Object **errptr, struct Object **args, size_t nargs)
{
	assert(nargs == 1);    // TODO: better argument check
	OBJECT_INCREF(ctx->interp, args[0]);   // we're returning a reference
	return args[0];
}

static struct Object *to_debug_string(struct Context *ctx, struct Object **errptr, struct Object **args, size_t nargs)
{
	assert(nargs == 1);    // TODO: better argument check

	struct UnicodeString noquotes = *((struct UnicodeString*) args[0]->data);
	struct UnicodeString yesquotes;
	yesquotes.len = noquotes.len + 2;
	yesquotes.val = malloc(sizeof(uint32_t) * yesquotes.len);
	if (!yesquotes.val) {
		*errptr = ctx->interp->nomemerr;
		return NULL;
	}
	yesquotes.val[0] = yesquotes.val[yesquotes.len - 1] = '"';
	memcpy(yesquotes.val + 1, noquotes.val, sizeof(uint32_t) * noquotes.len);

	struct Object *res = stringobject_newfromustr(ctx->interp, errptr, yesquotes);
	free(yesquotes.val);
	return res;
}

int stringobject_addmethods(struct Interpreter *interp, struct Object **errptr)
{
	struct Object *stringclass = interpreter_getbuiltin(interp, errptr, "String");
	if (!stringclass)
		return STATUS_ERROR;

	if (method_add(interp, errptr, stringclass, "to_string", to_string) == STATUS_ERROR) goto error;
	if (method_add(interp, errptr, stringclass, "to_debug_string", to_debug_string) == STATUS_ERROR) goto error;

	OBJECT_DECREF(interp, stringclass);
	return STATUS_OK;

error:
	OBJECT_DECREF(interp, stringclass);
	return STATUS_ERROR;
}

struct Object *stringobject_newfromustr(struct Interpreter *interp, struct Object **errptr, struct UnicodeString ustr)
{
	struct UnicodeString *data = unicodestring_copy(ustr);
	if (!data) {
		*errptr = interp->nomemerr;
		return NULL;
	}

	struct Object *stringclass = interpreter_getbuiltin(interp, errptr, "String");
	if (!stringclass) {
		free(data->val);
		free(data);
		return NULL;
	}

	struct Object *str = classobject_newinstance(interp, errptr, stringclass, data);
	OBJECT_DECREF(interp, stringclass);
	if (!str) {
		free(data->val);
		free(data);
		return NULL;
	}
	return str;
}

struct Object *stringobject_newfromcharptr(struct Interpreter *interp, struct Object **errptr, char *ptr)
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

	struct Object *stringclass = interpreter_getbuiltin(interp, errptr, "String");
	if (!stringclass) {
		free(data->val);
		free(data);
		return NULL;
	}

	struct Object *str = classobject_newinstance(interp, errptr, stringclass, data);
	OBJECT_DECREF(interp, stringclass);
	if (!str) {
		free(data->val);
		free(data);
		return NULL;
	}
	return str;
}
