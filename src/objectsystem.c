#include "objectsystem.h"
#include <assert.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include "common.h"
#include "hashtable.h"
#include "unicode.h"


static int compare_unicode_strings(void *a, void *b, void *userdata)
{
	assert(!userdata);
	struct UnicodeString *astr=a, *bstr=b;
	if (astr->len != bstr->len)
		return 0;

	// memcmp is not reliable :( https://stackoverflow.com/a/11995514
	for (size_t i=0; i < astr->len; i++) {
		if (astr->val[i] != bstr->val[i])
			return 0;
	}
	return 1;
}


struct ObjectClassInfo *object_newclass(struct ObjectClassInfo *base, objectclassinfo_destructor_t destructor)
{
	struct ObjectClassInfo *res = malloc(sizeof(struct ObjectClassInfo));
	if (!res)
		return NULL;
	res->methods = hashtable_new(compare_unicode_strings);
	if (!res->methods){
		free(res);
		return NULL;
	}
	res->baseclass = base;
	res->destructor = destructor;
	return res;
}


struct ObjectClassInfo *objectclassinfo_new(struct ObjectClassInfo *base, objectclassinfo_destructor_t destructor)
{
	struct ObjectClassInfo *res = malloc(sizeof(struct ObjectClassInfo));
	if (!res)
		return NULL;
	res->methods = hashtable_new(compare_unicode_strings);
	if (!res->methods){
		free(res);
		return NULL;
	}
	res->baseclass = base;
	res->destructor = destructor;
	return res;
}


void objectclassinfo_free(struct ObjectClassInfo *klass)
{
	hashtable_free(klass->methods);   // TODO: decref the values or something?
	free(klass);
}


struct Object *object_new(struct ObjectClassInfo *klass)
{
	struct Object *obj = malloc(sizeof(struct Object));
	if(!obj)
		return NULL;

	obj->klass = klass;
	obj->attrs = hashtable_new(compare_unicode_strings);
	if (!obj->attrs) {
		free(obj);
		return NULL;
	}
	obj->data = NULL;
	return obj;
}

int object_free(struct Object *obj)
{
	int status = STATUS_OK;
	if (obj->klass->destructor)
		status = obj->klass->destructor(obj);

	// keep going anyway, don't care about the destructor's status yet
	hashtable_free(obj->attrs);   // TODO: decref the values or something?
	free(obj);
	return status;
}

// TODO: create a wrapper object instead of adding the class info directly
int objectsystem_addbuiltinclass(struct HashTable *builtins, char *name, struct ObjectClassInfo *klass)
{
	char decodeerror[100] = {0};
	struct UnicodeString *uname = malloc(sizeof(struct UnicodeString));
	if (!uname)
		return STATUS_NOMEM;
	int res = utf8_decode(name, strlen(name), uname, decodeerror);
	assert(res != 1);   // the name must be valid utf8
	assert(decodeerror[0] == 0);
	if (res != STATUS_OK) {
		free(uname);
		return res;
	}

	if ((res = hashtable_set(builtins, uname, unicodestring_hash(*uname), klass, NULL)) != STATUS_OK) {
		free(uname->val);
		free(uname);
		return res;
	}
	return STATUS_OK;
}

int objectsystem_getbuiltin(struct HashTable *builtins, char *name, void **res)
{
	char decodeerror[100] = {0};
	struct UnicodeString uname;
	int ret = utf8_decode(name, strlen(name), &uname, decodeerror);
	assert(ret != 1);   // name must be valid utf8
	if (ret != STATUS_OK)
		return ret;

	ret = hashtable_get(builtins, &uname, unicodestring_hash(uname), res, NULL);
	free(uname.val);
	return ret;
}

struct Object *object_getmethod(struct ObjectClassInfo *klass, struct UnicodeString name)
{
	struct Object *res = NULL;
	int found = hashtable_get(klass->methods, &name, unicodestring_hash(name), (void **)(&res), NULL);
	assert(found==0 || found==1);
	assert(found ? !!res : !res);
	return res;
}
