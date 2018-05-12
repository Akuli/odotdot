#include "classobject.h"
#include <assert.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include "errors.h"
#include "../hashtable.h"
#include "../interpreter.h"
#include "../objectsystem.h"
#include "../unicode.h"

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

struct Object *classobject_new_noerrptr(struct Interpreter *interp, char *name, struct Object *base, int instanceshaveattrs, void (*foreachref)(struct Object*, void*, classobject_foreachrefcb), void (*destructor)(struct Object *instance))
{
	assert(instanceshaveattrs == 0 || instanceshaveattrs == 1);
	struct ClassObjectData *data = malloc(sizeof(struct ClassObjectData));
	if (!data)
		return NULL;

	data->methods = hashtable_new(compare_unicode_strings);
	if (!data->methods) {
		free(data);
		return NULL;
	}

	strncpy(data->name, name, 10);
	data->name[9] = 0;
	data->baseclass = base;
	if (base)
		OBJECT_INCREF(interp, base);
	data->instanceshaveattrs = instanceshaveattrs;
	data->foreachref = foreachref;
	data->destructor = destructor;

	// interp->classclass can be NULL, see builtins_setup()
	struct Object *klass = object_new(interp, interp->classclass, data);
	if (!klass) {
		OBJECT_DECREF(interp, base);
		hashtable_free(data->methods);
		free(data);
		return NULL;
	}

	return klass;
}

struct Object *classobject_new(struct Interpreter *interp, struct Object **errptr, char *name, struct Object *base, int instanceshaveattrs, void (*foreachref)(struct Object*, void*, classobject_foreachrefcb), void (*destructor)(struct Object *instance))
{
	assert(interp->classclass);
	assert(interp->nomemerr);

	if (!classobject_instanceof(base->klass, interp->classclass)) {
		// TODO: test this
		// FIXME: don't use builtinctx, instead require passing in a context to this function
		errorobject_setwithfmt(interp->builtinctx, errptr, "cannot inherit a new class from %D", base);
		return NULL;
	}

	struct Object *klass = classobject_new_noerrptr(interp, name, base, instanceshaveattrs, foreachref, destructor);
	if (!klass) {
		errorobject_setnomem(interp, errptr);
		return NULL;
	}
	return klass;
}

struct Object *classobject_newinstance(struct Interpreter *interp, struct Object **errptr, struct Object *klass, void *data)
{
	if (!classobject_instanceof(klass->klass, interp->classclass)) {
		// TODO: test this
		// FIXME: don't use builtinctx, instead require passing in a context to this function
		errorobject_setwithfmt(interp->builtinctx, errptr, "cannot create an instance of %D", klass);
		return NULL;
	}
	struct Object *instance = object_new(interp, klass, data);

	if (!instance) {
		errorobject_setnomem(interp, errptr);
		return NULL;
	}
	return instance;
}

int classobject_instanceof(struct Object *obj, struct Object *klass)
{
	do {
		if (obj->klass == klass)
			return 1;
	} while ((klass = ((struct ClassObjectData*) klass->data)->baseclass));
	return 0;
}


static void class_foreachref(struct Object *klass, void *cbdata, classobject_foreachrefcb cb)
{
	struct ClassObjectData *data = klass->data;
	if (data->baseclass)
		cb(data->baseclass, cbdata);

	struct HashTableIterator methoditer;
	hashtable_iterbegin(data->methods, &methoditer);
	while (hashtable_iternext(&methoditer))
		cb(methoditer.value, cbdata);
}


// keys must be freed, but values are automagically decreffed because class_foreachref
static void free_key_ustr(void *key, void *valueobj, void *datanull)
{
	free(((struct UnicodeString *) key)->val);
	free(key);
}


void classobject_destructor(struct Object *klass)
{
	struct ClassObjectData *data = klass->data;    // casts implicitly
	// object_free_impl() takes care of many things because class_foreachref
	hashtable_fclear(data->methods, free_key_ustr, NULL);
	hashtable_free(data->methods);
	free(data);
}

struct Object *classobject_create_classclass(struct Interpreter *interp, struct Object *objectclass)
{
	// TODO: should the name of class objects be implemented as an attribute?
	return classobject_new_noerrptr(interp, "Class", objectclass, 0, class_foreachref, classobject_destructor);
}
