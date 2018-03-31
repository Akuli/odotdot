#include "interpreter.h"
#include <assert.h>
#include <string.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include "common.h"
#include "hashtable.h"
#include "objectsystem.h"
#include "unicode.h"

// mostly copy/pasted from objectsystem.c
// TODO: check types?
static int compare_string_objects(void *a, void *b, void *userdata)
{
	assert(userdata == NULL);
	struct UnicodeString *astr=((struct Object *)a)->data, *bstr=((struct Object *)b)->data;
	if (astr->len != bstr->len)
		return 0;

	// memcmp is not reliable :( https://stackoverflow.com/a/11995514
	for (size_t i=0; i < astr->len; i++) {
		if (astr->val[i] != bstr->val[i])
			return 0;
	}
	return 1;
}

struct Interpreter *interpreter_new(void)
{
	struct Interpreter *interp = malloc(sizeof(struct Interpreter));
	if (!interp)
		return NULL;

	interp->globalvars = hashtable_new(compare_string_objects);
	if (!(interp->globalvars)) {
		free(interp);
		return NULL;
	}

	interp->nomemerr = NULL;
	return interp;
}

void interpreter_free(struct Interpreter *interp)
{
	hashtable_free(interp->globalvars);
	free(interp);
}

int interpreter_addglobal(struct Interpreter *interp, struct Object **errptr, char *name, struct Object *val)
{
	struct UnicodeString *uname = malloc(sizeof(struct UnicodeString));
	if(!uname) {
		*errptr = interp->nomemerr;
		return STATUS_ERROR;
	}

	char errormsg[100] = {0};
	int res = utf8_decode(name, strlen(name), uname, errormsg);
	if (res != STATUS_OK) {
		assert(res == STATUS_NOMEM);    // name must be valid utf8
		free(uname);
		*errptr = interp->nomemerr;
		return STATUS_ERROR;
	}
	assert(strlen(errormsg) == 0);

	res = hashtable_set(interp->globalvars, uname, unicodestring_hash(*uname), val, NULL);
	if (res != STATUS_OK) {
		assert(res == STATUS_NOMEM);    // compare_string_objects() shouldn't fail
		free(uname);
		*errptr = interp->nomemerr;
		return STATUS_ERROR;
	}

	return STATUS_OK;
}

struct Object *interpreter_getglobal(struct Interpreter *interp, struct Object **errptr, char *name)
{
	// no need to malloc and stuff because hashtable_get() doesn't store this anywhere
	struct UnicodeString uname;

	char errormsg[100] = {0};
	int res = utf8_decode(name, strlen(name), &uname, errormsg);
	if(res != STATUS_OK) {
		assert(res == STATUS_NOMEM);
		*errptr = interp->nomemerr;
		return NULL;
	}
	assert(strlen(errormsg) == 0);

	struct Object *val;
	int found = hashtable_get(interp->globalvars, &uname, unicodestring_hash(uname), (void **) &val, NULL);
	assert(found == 1);    // TODO: handle missing builtins better
	return val;
}
