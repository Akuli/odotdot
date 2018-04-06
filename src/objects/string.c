// most of the interesting stuff is implemented in ../unicode.c, this is just a wrapper

#include <assert.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include "classobject.h"
#include "../common.h"
#include "../interpreter.h"
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
