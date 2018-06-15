// https://en.wikipedia.org/wiki/Hash_table
// TODO: test more stuff

#include "mapping.h"
#include <assert.h>
#include <limits.h>
#include <stdlib.h>
#include "../attribute.h"
#include "../check.h"
#include "../equals.h"
#include "../method.h"
#include "array.h"
#include "classobject.h"
#include "errors.h"
#include "integer.h"
#include "null.h"
#include "string.h"

// hash should be a signed long, nbuckets should be an unsigned long
// casting signed to unsigned is apparently well-defined 0_o
#define HASH_MODULUS(hash, nbuckets) ( (unsigned long)(hash) % (unsigned long)(nbuckets) )


struct MappingObjectItem {
	struct Object *key;
	struct Object *value;
	struct MappingObjectItem *next;
};

static void mapping_destructor(struct Object *map)
{
	struct MappingObjectData *data = map->data;
	for (unsigned long i=0; i < data->nbuckets; i++) {
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

static struct MappingObjectData *create_empty_data(void)
{
	struct MappingObjectData *data = malloc(sizeof(struct MappingObjectData));
	if (!data)
		return NULL;

	data->size = 0;
	data->nbuckets = 50;
	data->buckets = calloc(data->nbuckets, sizeof(struct MappingObjectItem));
	if (!(data->buckets)) {
		free(data);
		return NULL;
	}
	return data;
}

struct Object *mappingobject_newempty(struct Interpreter *interp)
{
	struct MappingObjectData *data = create_empty_data();
	if (!data) {
		errorobject_setnomem(interp);
		return NULL;
	}

	struct Object *map = classobject_newinstance(interp, interp->builtins.Mapping, data, mapping_destructor);
	if (!map) {
		free(data->buckets);
		free(data);
		return NULL;
	}

	map->hashable = false;
	return map;
}


static struct Object *setup(struct Interpreter *interp, struct Object *args, struct Object *opts)
{
	struct Object *map, *pairs;
	if (ARRAYOBJECT_LEN(args) == 1) {
		// (new Mapping)
		if (!check_args(interp, args, interp->builtins.Mapping, NULL)) return NULL;
		map = ARRAYOBJECT_GET(args, 0);
		pairs = NULL;
	} else {
		// (new Mapping pairs)
		if (!check_args(interp, args, interp->builtins.Mapping, interp->builtins.Array, NULL)) return NULL;
		map = ARRAYOBJECT_GET(args, 0);
		pairs = ARRAYOBJECT_GET(args, 1);
	}

	if (map->data) {
		errorobject_throwfmt(interp, "AssertError", "setup was called twice");
		return NULL;
	}
	struct MappingObjectData *data = create_empty_data();
	if (!data) {
		errorobject_setnomem(interp);
		return NULL;
	}

	map->data = data;
	map->destructor = mapping_destructor;
	map->hashable = false;

	// TODO: throw an error if there are duplicates?

	if (pairs) {
		for (size_t i=0; i < ARRAYOBJECT_LEN(pairs); i++) {
			struct Object *pair = ARRAYOBJECT_GET(pairs, i);
			if (!classobject_isinstanceof(pair, interp->builtins.Array)) {
				errorobject_throwfmt(interp, "TypeError", "expected a [key value] pair, got %D", pair);
				return NULL;
			}
			if (ARRAYOBJECT_LEN(pair) != 2) {
				errorobject_throwfmt(interp, "ValueError", "expected a [key value] pair, got %D", pair);
				return NULL;
			}
			if (!mappingobject_set(interp, map, ARRAYOBJECT_GET(pair, 0), ARRAYOBJECT_GET(pair, 1)))
				return NULL;
		}
	}

	// add opts to the mapping
	struct MappingObjectIter iter;
	mappingobject_iterbegin(&iter, opts);
	while (mappingobject_iternext(&iter)) {
		if (!mappingobject_set(interp, map, iter.key, iter.value))
			return NULL;
	}

	return nullobject_get(interp);
}


static struct Object *length_getter(struct Interpreter *interp, struct Object *args, struct Object *opts)
{
	if (!check_args(interp, args, interp->builtins.Mapping, NULL)) return NULL;
	if (!check_no_opts(interp, opts)) return NULL;
	return integerobject_newfromlonglong(interp, MAPPINGOBJECT_SIZE(ARRAYOBJECT_GET(args, 0)));
}


static bool make_bigger(struct Interpreter *interp, struct MappingObjectData *data)
{
	assert(data->nbuckets != ULONG_MAX);   // the caller should check this

	unsigned long newnbuckets = (data->nbuckets > ULONG_MAX/3) ? ULONG_MAX : data->nbuckets*3;

	struct MappingObjectItem **newbuckets = calloc(newnbuckets, sizeof(struct MappingObjectItem));
	if (!newbuckets) {
		errorobject_setnomem(interp);
		return false;
	}

	for (unsigned long oldi = 0; oldi < data->nbuckets; oldi++) {
		struct MappingObjectItem *next;
		for (struct MappingObjectItem *item = data->buckets[oldi]; item; item=next) {
			next = item->next;   // must be before changing item->next

			// put the item to a new bucket, same code as in set()
			unsigned long newi = HASH_MODULUS(item->key->hash, newnbuckets);
			item->next = newbuckets[newi];   // may be NULL
			newbuckets[newi] = item;
		}
	}

	free(data->buckets);
	data->buckets = newbuckets;
	data->nbuckets = newnbuckets;
	return true;
}

bool hashable_check(struct Interpreter *interp, struct Object *key)
{
	if (!(key->hashable)) {
		struct ClassObjectData *keyclassdata = key->klass->data;
		errorobject_throwfmt(interp, "TypeError", "%U objects are not hashable, so %D can't be used as a Mapping key", keyclassdata->name, key);
		return false;
	}
	return true;
}

bool mappingobject_set(struct Interpreter *interp, struct Object *map, struct Object *key, struct Object *val)
{
	if (!hashable_check(interp, key))
		return false;

	struct MappingObjectData *data = map->data;
	unsigned long i = HASH_MODULUS(key->hash, data->nbuckets);
	for (struct MappingObjectItem *olditem = data->buckets[i]; olditem; olditem=olditem->next) {
		int eqres = equals(interp, olditem->key, key);
		if (eqres == -1)
			return false;
		if (eqres == 1) {
			// the key is already in the mapping, update the value
			OBJECT_DECREF(interp, olditem->value);
			olditem->value = val;
			OBJECT_INCREF(interp, val);
			return true;
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
	if (data->nbuckets != LONG_MAX) {
		float loadfactor = ((float) data->size) / data->nbuckets;
		if (loadfactor > 0.75) {
			if (!make_bigger(interp, data))
				return false;
			i = HASH_MODULUS(key->hash, data->nbuckets);
		}
	}

	struct MappingObjectItem *item = malloc(sizeof(struct MappingObjectItem));
	if (!item) {
		errorobject_setnomem(interp);
		return false;
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
	return true;
}

static struct Object *set(struct Interpreter *interp, struct Object *args, struct Object *opts)
{
	if (!check_args(interp, args, interp->builtins.Mapping, interp->builtins.Object, interp->builtins.Object, NULL)) return NULL;
	if (!check_no_opts(interp, opts)) return NULL;
	if (!mappingobject_set(interp, ARRAYOBJECT_GET(args, 0), ARRAYOBJECT_GET(args, 1), ARRAYOBJECT_GET(args, 2))) return NULL;
	return nullobject_get(interp);
}


int mappingobject_get(struct Interpreter *interp, struct Object *map, struct Object *key, struct Object **val)
{
	if (!hashable_check(interp, key))
		return -1;

	struct MappingObjectData *data = map->data;
	for (struct MappingObjectItem *item = data->buckets[HASH_MODULUS(key->hash, data->nbuckets)]; item; item=item->next) {
		int eqres = equals(interp, item->key, key);
		if (eqres == -1)
			return -1;
		if (eqres == 1) {
			OBJECT_INCREF(interp, item->value);
			*val = item->value;
			return 1;
		}
		assert(eqres == 0);
	}
	return 0;
}

static struct Object *get(struct Interpreter *interp, struct Object *args, struct Object *opts)
{
	if (!check_args(interp, args, interp->builtins.Mapping, interp->builtins.Object, NULL)) return NULL;
	if (!check_no_opts(interp, opts)) return NULL;
	struct Object *map = ARRAYOBJECT_GET(args, 0);
	struct Object *key = ARRAYOBJECT_GET(args, 1);

	struct Object *val;
	int status = mappingobject_get(interp, map, key, &val);
	if (status == 0)
		errorobject_throwfmt(interp, "KeyError", "cannot find key %D", key);
	if (status != 1)
		return NULL;
	return val;
}


static struct Object *get_and_delete(struct Interpreter *interp, struct Object *args, struct Object *opts)
{
	if (!check_args(interp, args, interp->builtins.Mapping, interp->builtins.Object, NULL)) return NULL;
	if (!check_no_opts(interp, opts)) return NULL;
	struct Object *map = ARRAYOBJECT_GET(args, 0);
	struct Object *key = ARRAYOBJECT_GET(args, 1);

	if (!hashable_check(interp, key))
		return NULL;

	struct MappingObjectData *data = map->data;
	unsigned long i = HASH_MODULUS(key->hash, data->nbuckets);

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
	errorobject_throwfmt(interp, "KeyError", "cannot find key %D", key);
	return NULL;
}

static struct Object *delete(struct Interpreter *interp, struct Object *args, struct Object *opts)
{
	struct Object *res = get_and_delete(interp, args, opts);
	if (!res)
		return NULL;

	OBJECT_DECREF(interp, res);
	return nullobject_get(interp);
}


bool mappingobject_addmethods(struct Interpreter *interp)
{
	if (!attribute_add(interp, interp->builtins.Mapping, "length", length_getter, NULL)) return false;
	if (!method_add(interp, interp->builtins.Mapping, "setup", setup)) return false;
	if (!method_add(interp, interp->builtins.Mapping, "set", set)) return false;
	if (!method_add(interp, interp->builtins.Mapping, "get", get)) return false;
	if (!method_add(interp, interp->builtins.Mapping, "delete", delete)) return false;
	if (!method_add(interp, interp->builtins.Mapping, "get_and_delete", get_and_delete)) return false;
	// TODO: to_debug_string
	return true;
}


void mappingobject_iterbegin(struct MappingObjectIter *it, struct Object *map)
{
	it->data = map->data;
	it->started = 0;
	it->lastbucketno = 0;
}

bool mappingobject_iternext(struct MappingObjectIter *it)
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
	if (!map->data)
		return;

	struct MappingObjectIter iter;
	mappingobject_iterbegin(&iter, map);
	while (mappingobject_iternext(&iter)) {
		cb(iter.key, cbdata);
		cb(iter.value, cbdata);
	}
}

struct Object *mappingobject_createclass(struct Interpreter *interp)
{
	return classobject_new(interp, "Mapping", interp->builtins.Object, foreachref, false);
}
