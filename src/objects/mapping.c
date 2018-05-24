// https://en.wikipedia.org/wiki/Hash_table
// TODO: test everything

#include "mapping.h"
#include <assert.h>
#include <limits.h>
#include <stdlib.h>
#include "../common.h"
#include "../equals.h"
#include "../method.h"
#include "classobject.h"
#include "errors.h"
#include "string.h"

struct MappingObjectItem {
	struct Object *key;
	struct Object *value;
	struct MappingObjectItem *next;
};

static void mapping_destructor(struct Object *map)
{
	struct MappingObjectData *data = map->data;
	for (size_t i=0; i < data->nbuckets; i++) {
		struct MappingObjectItem *item = data->buckets[i];
		while (item) {
			void *gonnafree = item;
			item = item->next;   // must be before the free
			free(gonnafree);
		}
	}
	free(data->buckets);
	free(data);
}

struct Object *mappingobject_newempty(struct Interpreter *interp)
{
	struct MappingObjectData *data = malloc(sizeof(struct MappingObjectData));
	if (!data) {
		errorobject_setnomem(interp);
		return NULL;
	}

	data->size = 0;
	data->nbuckets = 50;
	data->buckets = calloc(data->nbuckets, sizeof(struct MappingObjectItem));
	if (!(data->buckets)) {
		errorobject_setnomem(interp);
		free(data);
		return NULL;
	}

	struct Object *map = classobject_newinstance(interp, interp->builtins.mappingclass, data, mapping_destructor);
	if (!map) {
		free(data->buckets);
		free(data);
		return NULL;
	}

	map->hashable = 0;
	return map;
}

static int make_bigger(struct Interpreter *interp, struct MappingObjectData *data)
{
	assert(data->nbuckets != UINT_MAX);   // the caller should check this

	unsigned int newnbuckets = (data->nbuckets > UINT_MAX/3) ? UINT_MAX : data->nbuckets*3;

	struct MappingObjectItem **newbuckets = calloc(newnbuckets, sizeof(struct MappingObjectItem));
	if (!newbuckets) {
		errorobject_setnomem(interp);
		return STATUS_ERROR;
	}

	for (size_t oldi = 0; oldi < data->nbuckets; oldi++) {
		struct MappingObjectItem *next;
		for (struct MappingObjectItem *item = data->buckets[oldi]; item; item=next) {
			next = item->next;   // must be before changing item->next

			// put the item to a new bucket, same code as in set()
			size_t newi = item->key->hash % newnbuckets;
			item->next = newbuckets[newi];   // may be NULL
			newbuckets[newi] = item;
		}
	}

	free(data->buckets);
	data->buckets = newbuckets;
	data->nbuckets = newnbuckets;
	return STATUS_OK;
}


int mappingobject_set(struct Interpreter *interp, struct Object *map, struct Object *key, struct Object *val)
{
	if (!(key->hashable)) {
		errorobject_setwithfmt(interp, "%D is not hashable, so it can't be used as a Mapping key", key);
		return STATUS_ERROR;
	}

	struct MappingObjectData *data = map->data;
	unsigned int i = key->hash % data->nbuckets;
	for (struct MappingObjectItem *olditem = data->buckets[i]; olditem; olditem=olditem->next) {
		int eqres = equals(interp, olditem->key, key);
		if (eqres == -1)
			return STATUS_ERROR;
		if (eqres == 1) {
			// the key is already in the mapping, update the value
			OBJECT_DECREF(interp, olditem->value);
			olditem->value = val;
			OBJECT_INCREF(interp, val);
			return STATUS_OK;
		}
		assert(eqres == 0);
	}

	/*
	now we know that we need to add a new MappingObjectItem
	https://en.wikipedia.org/wiki/Hash_table#Key_statistics
	https://en.wikipedia.org/wiki/Hash_table#Dynamic_resizing
	try changing the load factor and running misc/hashtable_speedtest.c
	java's 3/4 seems to be a really good choice for this value, at least on my system
	TODO: modify hashtable_speedtest.c for this
	*/
	if (data->nbuckets != UINT_MAX) {
		float loadfactor = ((float) data->size) / data->nbuckets;
		if (loadfactor > 0.75) {
			if (make_bigger(interp, data) == STATUS_ERROR)
				return STATUS_ERROR;
			i = key->hash % data->nbuckets;
		}
	}

	struct MappingObjectItem *item = malloc(sizeof(struct MappingObjectItem));
	if (!item) {
		errorobject_setnomem(interp);
		return STATUS_ERROR;
	}
	item->key = key;
	OBJECT_INCREF(interp, key);
	item->value = val;
	OBJECT_INCREF(interp, val);

	// it's easier to insert this item before the old item
	// i learned this from k&r
	item->next = data->buckets[i];    // may be NULL
	data->buckets[i] = item;
	data->size++;
	return STATUS_OK;
}

static struct Object *set(struct Interpreter *interp, struct Object **args, size_t nargs)
{
	if (functionobject_checktypes(interp, args, nargs, interp->builtins.mappingclass, interp->builtins.objectclass, interp->builtins.objectclass, NULL) == STATUS_ERROR)
		return NULL;
	if (mappingobject_set(interp, args[0], args[1], args[2]) == STATUS_ERROR)
		return NULL;

	// must return a new reference
	// TODO: we need a nulll ::((((((
	return stringobject_newfromcharptr(interp, "asd");
}


struct Object *mappingobject_get(struct Interpreter *interp, struct Object *map, struct Object *key)
{
	if (!(key->hashable)) {
		errorobject_setwithfmt(interp, "%D is not hashable, so it can't be used as a Mapping key", key);
		return NULL;
	}

	struct MappingObjectData *data = map->data;
	for (struct MappingObjectItem *item = data->buckets[key->hash % data->nbuckets]; item; item=item->next) {
		int eqres = equals(interp, item->key, key);
		if (eqres == -1)
			return NULL;
		if (eqres == 1) {
			OBJECT_INCREF(interp, item->value);
			return item->value;
		}
		assert(eqres == 0);
	}

	// returns NULL without setting err... be careful with this
	return NULL;
}

static struct Object *get(struct Interpreter *interp, struct Object **args, size_t nargs)
{
	if (functionobject_checktypes(interp, args, nargs, interp->builtins.mappingclass, interp->builtins.objectclass, NULL) == STATUS_ERROR)
		return NULL;
	struct Object *map = args[0];
	struct Object *key = args[1];

	assert(!(interp->err));
	struct Object *res = mappingobject_get(interp, map, key);
	if (!res && !(interp->err))
		errorobject_setwithfmt(interp, "cannot find key %D", key);
	return res;
}


static struct Object *get_and_delete(struct Interpreter *interp, struct Object **args, size_t nargs)
{
	if (functionobject_checktypes(interp, args, nargs, interp->builtins.mappingclass, interp->builtins.objectclass, NULL) == STATUS_ERROR)
		return NULL;
	struct Object *map = args[0];
	struct Object *key = args[1];

	if (!(key->hashable)) {
		errorobject_setwithfmt(interp, "%D is not hashable, so it can't be used as a Mapping key", key);
		return NULL;
	}

	struct MappingObjectData *data = map->data;
	unsigned int i = key->hash % data->nbuckets;

	struct MappingObjectItem *prev = NULL;
	for (struct MappingObjectItem *item = data->buckets[i]; item; item=item->next) {
		int eqres = equals(interp, item->key, key);
		if (eqres == -1)
			return NULL;
		if (eqres == 1) {
			// if you modify this, keep in mind that item->next may be NULL
			if (prev)   // skip this item
				prev->next = item->next;
			else      // this is the first item of the bucket
				data->buckets[i] = item->next;
			data->size--;

			OBJECT_DECREF(interp, item->key);
			struct Object *res = item->value;   // don't decref this, the reference is returned
			free(item);
			return res;
		}
		assert(eqres == 0);
		prev = item;
	}

	// empty buckets or nothing but hash collisions
	errorobject_setwithfmt(interp, "cannot find key %D", key);
	return NULL;
}

static struct Object *delete(struct Interpreter *interp, struct Object **args, size_t nargs)
{
	struct Object *res = get_and_delete(interp, args, nargs);
	if (!res)
		return NULL;
	OBJECT_DECREF(interp, res);

	// must return a new reference (lol)
	return stringobject_newfromcharptr(interp, "asd");
}


void mappingobject_iterbegin(struct MappingObjectIter *it, struct Object *map)
{
	it->data = map->data;
	it->started = 0;
	it->lastbucketno = 0;
}

int mappingobject_iternext(struct MappingObjectIter *it)
{
	if (!(it->started)) {
		it->lastbucketno = 0;
		it->started = 1;
		goto starthere;     // OMG OMG ITS A GOTO KITTENS DIE OR SOMETHING
	}
	if (it->lastbucketno == it->data->nbuckets)   // the end
		return 0;

	if (it->lastitem->next) {
		it->lastitem = it->lastitem->next;
	} else {
		it->lastbucketno++;
starthere:
		while (it->lastbucketno < it->data->nbuckets && !(it->data->buckets[it->lastbucketno]))
			it->lastbucketno++;
		if (it->lastbucketno == it->data->nbuckets)   // the end
			return 0;
		it->lastitem = it->data->buckets[it->lastbucketno];
	}

	it->key = it->lastitem->key;
	it->value = it->lastitem->value;
	return 1;
}


static void foreachref(struct Object *map, void *cbdata, classobject_foreachrefcb cb)
{
	struct MappingObjectIter iter;
	mappingobject_iterbegin(&iter, map);
	while (mappingobject_iternext(&iter)) {
		cb(iter.key, cbdata);
		cb(iter.value, cbdata);
	}
}

struct Object *mappingobject_createclass(struct Interpreter *interp)
{
	return classobject_new(interp, "Mapping", interp->builtins.objectclass, 0, foreachref);
}

int mappingobject_addmethods(struct Interpreter *interp)
{
	if (method_add(interp, interp->builtins.mappingclass, "set", set) == STATUS_ERROR) return STATUS_ERROR;
	if (method_add(interp, interp->builtins.mappingclass, "get", get) == STATUS_ERROR) return STATUS_ERROR;
	if (method_add(interp, interp->builtins.mappingclass, "delete", delete) == STATUS_ERROR) return STATUS_ERROR;
	if (method_add(interp, interp->builtins.mappingclass, "get_and_delete", get_and_delete) == STATUS_ERROR) return STATUS_ERROR;
	return STATUS_OK;
}
