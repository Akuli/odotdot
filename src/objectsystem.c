#include "objectsystem.h"
#include <assert.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include "common.h"
#include "hashtable.h"
#include "interpreter.h"
#include "unicode.h"

// the values of interp->allobjects are &dummy
static int dummy = 123;

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


struct ObjectClassInfo *objectclassinfo_new(char *name, struct ObjectClassInfo *base, objectclassinfo_foreachref foreachref, void (*destructor)(struct Object *))
{
	struct ObjectClassInfo *res = malloc(sizeof(struct ObjectClassInfo));
	if (!res)
		return NULL;
	res->methods = hashtable_new(compare_unicode_strings);
	if (!res->methods){
		free(res);
		return NULL;
	}

	strncpy(res->name, name, 10);
	res->name[9] = 0;
	res->baseclass = base;
	res->foreachref = foreachref;
	res->destructor = destructor;
	return res;
}


void objectclassinfo_free(struct ObjectClassInfo *klass)
{
	hashtable_clear(klass->methods);   // TODO: decref the values or something?
	hashtable_free(klass->methods);
	free(klass);
}


struct Object *object_new(struct Interpreter *interp, struct Object *klass, void *data)
{
	struct Object *obj = malloc(sizeof(struct Object));
	if(!obj)
		return NULL;

	obj->attrs = hashtable_new(compare_unicode_strings);
	if (!obj->attrs) {
		free(obj);
		return NULL;
	}

	obj->klass = klass;
	obj->data = data;
	obj->refcount = 1;   // the returned reference
	obj->gcflag = 0;

	if (hashtable_set(interp->allobjects, obj, (unsigned int)((uintptr_t)obj), &dummy, NULL) == STATUS_NOMEM) {
		hashtable_free(obj->attrs);
		free(obj);
		return NULL;
	}
	return obj;
}

static void decref_the_ref(struct Object *ref, void *interpdata)
{
	OBJECT_DECREF(interpdata, ref);
}

void object_free_impl(struct Interpreter *interp, struct Object *obj)
{
	// these two asserts make sure that the sign of the refcount is shown in error messages
	assert(obj->refcount >= 0);
	assert(obj->refcount <= 0);

	// obj->klass can be set to NULL to prevent running stuff
	// see builtins_teardown() for an example
	if (obj->klass) {
		struct ObjectClassInfo *classinfo = obj->klass->data;

		// TODO: document the order that these are called in
		// it really makes a difference with e.g. Array
		if (classinfo->foreachref)
			classinfo->foreachref(obj, interp, decref_the_ref);
		if (classinfo->destructor)
			classinfo->destructor(obj);

		OBJECT_DECREF(interp, obj->klass);
	}

	void *dummyptr;
	assert(hashtable_pop(interp->allobjects, obj, (unsigned int)((uintptr_t)obj), &dummyptr, NULL) == 1);

	hashtable_clear(obj->attrs);   // TODO: decref the values or something?
	hashtable_free(obj->attrs);
	free(obj);
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
