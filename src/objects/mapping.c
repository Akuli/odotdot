// https://en.wikipedia.org/wiki/Hash_table

#include "mapping.h"
#include <assert.h>
#include <limits.h>
#include <stdlib.h>
#include "../attribute.h"
#include "../check.h"
#include "../interpreter.h"
#include "../objectsystem.h"
#include "../operator.h"
#include "../method.h"
#include "array.h"
#include "classobject.h"
#include "errors.h"
#include "function.h"
#include "integer.h"
#include "option.h"

// hash should be a signed long, nbuckets should be an unsigned long
// casting signed to unsigned is apparently well-defined 0_o
#define HASH_MODULUS(hash, nbuckets) ( (unsigned long)(hash) % (unsigned long)(nbuckets) )


struct MappingObjectItem {
	struct Object *key;
	struct Object *value;
	struct MappingObjectItem *next;
};

static void iterbegin_with_data(struct MappingObjectIter *it, struct MappingObjectData *data);   // fwd dcl
static void mapping_foreachref(void *data, object_foreachrefcb cb, void *cbdata)
{
	struct MappingObjectIter iter;
	iterbegin_with_data(&iter, data);
	while (mappingobject_iternext(&iter)) {
		cb(iter.key, cbdata);
		cb(iter.value, cbdata);
	}
}

static void mapping_destructor(void *data)
{
	for (unsigned long i=0; i < ((struct MappingObjectData *)data)->nbuckets; i++) {
		struct MappingObjectItem *item = ((struct MappingObjectData *)data)->buckets[i];
		while (item) {
			void *gonnafree = item;
			item = item->next;   // must be before the free
			free(gonnafree);
		}
	}
	free(((struct MappingObjectData *)data)->buckets);
	free(data);
}

static struct MappingObjectData *create_empty_data(void)
{
	struct MappingObjectData *data = malloc(sizeof(struct MappingObjectData));
	if (!data)
		return NULL;

	data->size = 0;
	data->nbuckets = 10;     // i experimented with different values, this was good
	data->buckets = calloc(data->nbuckets, sizeof(struct MappingObjectItem*));
	if (!(data->buckets)) {
		free(data);
		return NULL;
	}
	return data;
}

static struct Object *new_empty(struct Interpreter *interp, struct Object *klass)
{
	struct MappingObjectData *data = create_empty_data();
	if (!data) {
		errorobject_thrownomem(interp);
		return NULL;
	}

	struct Object *map = object_new_noerr(interp, klass, (struct ObjectData){.data=data, .foreachref=mapping_foreachref, .destructor=mapping_destructor});
	if (!map) {
		errorobject_thrownomem(interp);
		free(data->buckets);
		free(data);
		return NULL;
	}

	map->hashable = false;
	return map;
}

struct Object *mappingobject_newempty(struct Interpreter *interp) {
	return new_empty(interp, interp->builtins.Mapping);
}

static struct Object *newinstance(struct Interpreter *interp, struct Object *args, struct Object *opts)
{
	return new_empty(interp, ARRAYOBJECT_GET(args, 0));
}

struct Object *mappingobject_createclass(struct Interpreter *interp)
{
	return classobject_new(interp, "Mapping", interp->builtins.Object, newinstance);
}


static bool setup(struct Interpreter *interp, struct ObjectData thisdata, struct Object *args, struct Object *opts)
{
	struct Object *map = thisdata.data;
	if (ARRAYOBJECT_LEN(args) == 0) {
		// (new Mapping)
		// no need to do anything here
	} else {
		// (new Mapping pairs)
		// TODO: don't check_args here for a better error message
		// the error messages by this talk about needing to pass exactly 1 argument, but 0 args are also allowed
		if (!check_args(interp, args, interp->builtins.Array, NULL)) return false;

		struct Object *pairs = ARRAYOBJECT_GET(args, 0);
		for (size_t i=0; i < ARRAYOBJECT_LEN(pairs); i++) {
			struct Object *pair = ARRAYOBJECT_GET(pairs, i);
			if (!classobject_isinstanceof(pair, interp->builtins.Array)) {
				errorobject_throwfmt(interp, "TypeError", "expected a [key value] pair, got %D", pair);
				return false;
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

	return true;
}


static struct Object *length_getter(struct Interpreter *interp, struct ObjectData nulldata, struct Object *args, struct Object *opts)
{
	if (!check_args(interp, args, interp->builtins.Mapping, NULL)) return NULL;
	if (!check_no_opts(interp, opts)) return NULL;
	return integerobject_newfromlonglong(interp, MAPPINGOBJECT_SIZE(ARRAYOBJECT_GET(args, 0)));
}


static bool make_bigger(struct Interpreter *interp, struct MappingObjectData *data)
{
	assert(data->nbuckets != 0);           // should never happen
	assert(data->nbuckets != ULONG_MAX);   // the caller should check this

	unsigned long newnbuckets = (data->nbuckets > ULONG_MAX/3) ? ULONG_MAX : data->nbuckets*3;

	struct MappingObjectItem **newbuckets = calloc(newnbuckets, sizeof(struct MappingObjectItem*));
	if (!newbuckets) {
		errorobject_thrownomem(interp);
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

static bool hashable_check(struct Interpreter *interp, struct Object *key)
{
	if (!(key->hashable)) {
		struct ClassObjectData *keyclassdata = key->klass->objdata.data;
		errorobject_throwfmt(interp, "TypeError", "%U objects are not hashable, so %D can't be used as a Mapping key", keyclassdata->name, key);
		return false;
	}
	return true;
}

bool mappingobject_set(struct Interpreter *interp, struct Object *map, struct Object *key, struct Object *val)
{
	if (!hashable_check(interp, key))
		return false;

	struct MappingObjectData *data = map->objdata.data;
	unsigned long i = HASH_MODULUS(key->hash, data->nbuckets);
	for (struct MappingObjectItem *olditem = data->buckets[i]; olditem; olditem=olditem->next) {
		int eqres = operator_eqint(interp, olditem->key, key);
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
	TODO: update hashtable_speedtest.c, it's been a long time since i tested this
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
		errorobject_thrownomem(interp);
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

static bool set(struct Interpreter *interp, struct ObjectData thisdata, struct Object *args, struct Object *opts)
{
	if (!check_args(interp, args, interp->builtins.Object, interp->builtins.Object, NULL)) return false;
	if (!check_no_opts(interp, opts)) return false;
	struct Object *map = thisdata.data;
	struct Object *key = ARRAYOBJECT_GET(args, 0);
	struct Object *val = ARRAYOBJECT_GET(args, 1);
	return mappingobject_set(interp, map, key, val);
}


int mappingobject_get(struct Interpreter *interp, struct Object *map, struct Object *key, struct Object **val)
{
	if (!hashable_check(interp, key))
		return -1;

	struct MappingObjectData *data = map->objdata.data;
	for (struct MappingObjectItem *item = data->buckets[HASH_MODULUS(key->hash, data->nbuckets)]; item; item=item->next) {
		int eqres = operator_eqint(interp, item->key, key);
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

int mappingobject_getanddelete(struct Interpreter *interp, struct Object *map, struct Object *key, struct Object **val)
{
	if (!hashable_check(interp, key))
		return -1;

	struct MappingObjectData *data = map->objdata.data;
	unsigned long i = HASH_MODULUS(key->hash, data->nbuckets);

	struct MappingObjectItem *prev = NULL;
	for (struct MappingObjectItem *item = data->buckets[i]; item; item=item->next) {
		int eqres = operator_eqint(interp, item->key, key);
		if (eqres == -1)
			return -1;
		if (eqres == 1) {
			// if you modify this, keep in mind that item->next may be NULL
			if (prev)   // skip this item
				prev->next = item->next;
			else      // this is the first item of the bucket
				data->buckets[i] = item->next;
			data->size--;

			OBJECT_DECREF(interp, item->key);
			*val = item->value;   // don't decref this, the reference is put to *val
			free(item);
			return 1;
		}
		assert(eqres == 0);
		prev = item;
	}

	return 0;
}


static struct Object *get(struct Interpreter *interp, struct ObjectData thisdata, struct Object *args, struct Object *opts)
{
	if (!check_args(interp, args, interp->builtins.Object, NULL)) return NULL;
	if (!check_no_opts(interp, opts)) return NULL;
	struct Object *map = thisdata.data;
	struct Object *key = ARRAYOBJECT_GET(args, 0);

	struct Object *val;
	int status = mappingobject_get(interp, map, key, &val);
	if (status == 0)
		errorobject_throwfmt(interp, "KeyError", "cannot find key %D", key);
	if (status != 1)
		return NULL;
	return val;
}

static struct Object *get_and_delete(struct Interpreter *interp, struct ObjectData thisdata, struct Object *args, struct Object *opts)
{
	if (!check_args(interp, args, interp->builtins.Object, NULL)) return NULL;
	if (!check_no_opts(interp, opts)) return NULL;
	struct Object *map = thisdata.data;
	struct Object *key = ARRAYOBJECT_GET(args, 0);

	struct Object *val;
	int status = mappingobject_getanddelete(interp, map, key, &val);
	if (status == 0)
		errorobject_throwfmt(interp, "KeyError", "cannot find key %D", key);
	if (status != 1)
		return NULL;
	return val;
}

bool mappingobject_addmethods(struct Interpreter *interp)
{
	if (!attribute_add(interp, interp->builtins.Mapping, "length", length_getter, NULL)) return false;
	if (!method_add_noret(interp, interp->builtins.Mapping, "setup", setup)) return false;
	if (!method_add_noret(interp, interp->builtins.Mapping, "set", set)) return false;
	if (!method_add_yesret(interp, interp->builtins.Mapping, "get", get)) return false;
	if (!method_add_yesret(interp, interp->builtins.Mapping, "get_and_delete", get_and_delete)) return false;
	// TODO: to_debug_string
	return true;
}


static void iterbegin_with_data(struct MappingObjectIter *it, struct MappingObjectData *data)
{
	it->data = data;
	it->started = 0;
	it->lastbucketno = 0;
}

void mappingobject_iterbegin(struct MappingObjectIter *it, struct Object *map)
{
	iterbegin_with_data(it, map->objdata.data);
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


#define BOOL_OPTION(interp, b) optionobject_new((interp), (b) ? (interp)->builtins.yes : (interp)->builtins.no)

static struct Object *eq(struct Interpreter *interp, struct ObjectData nulldata, struct Object *args, struct Object *opts)
{
	if (!check_args(interp, args, interp->builtins.Object, interp->builtins.Object, NULL)) return NULL;
	if (!check_no_opts(interp, opts)) return NULL;

	struct Object *map1 = ARRAYOBJECT_GET(args, 0), *map2 = ARRAYOBJECT_GET(args, 1);
	if (!(classobject_isinstanceof(map1, interp->builtins.Mapping) && classobject_isinstanceof(map2, interp->builtins.Mapping))) {
		OBJECT_INCREF(interp, interp->builtins.none);
		return interp->builtins.none;
	}

	// mappings are equal if they have same keys and values
	// that's true if and only if for every key of a, a.get(key) == b.get(key)
	// unless a and b are of different lengths
	// or a and b have same number of keys but different keys, and .get() fails

	if (MAPPINGOBJECT_SIZE(map1) != MAPPINGOBJECT_SIZE(map2))
		return BOOL_OPTION(interp, false);

	struct MappingObjectIter iter;
	mappingobject_iterbegin(&iter, map1);
	while (mappingobject_iternext(&iter)) {
		struct Object *map2val;

		int res = mappingobject_get(interp, map2, iter.key, &map2val);
		if (res == -1)
			return NULL;
		if (res == 0)
			return BOOL_OPTION(interp, false);
		assert(res == 1);

		// ok, so we found the value... let's compare
		res = operator_eqint(interp, iter.value, map2val);
		OBJECT_DECREF(interp, map2val);
		if (res == -1)
			return NULL;
		if (res == 0)
			return BOOL_OPTION(interp, false);
		assert(res == 1);
	}

	// no differences found
	return BOOL_OPTION(interp, true);
}

bool mappingobject_initoparrays(struct Interpreter *interp) {
	return functionobject_add2array(interp, interp->oparrays.eq, "mapping_eq", functionobject_mkcfunc_yesret(eq));
}
