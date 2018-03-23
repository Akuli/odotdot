// https://en.wikipedia.org/wiki/Hash_table

#include "mapping.h"
#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include "common.h"

struct Mapping *mapping_new(mapping_cmpfunc keycmp)
{
	struct Mapping *map = malloc(sizeof(struct Mapping));
	if(!map)
		return NULL;
	map->keycmp = keycmp;
	map->size = 0;
	map->nbuckets = 50;
	map->buckets = calloc(map->nbuckets, sizeof(struct MappingItem));
	if(!map->buckets) {
		free(map);
		return NULL;
	}
	return map;
}


static int make_bigger(struct Mapping *);   // forward declaration

int mapping_set(struct Mapping *map, void *key, unsigned long keyhash, void *value, void *userdata)
{
	/*
	https://en.wikipedia.org/wiki/Hash_table#Key_statistics
	https://en.wikipedia.org/wiki/Hash_table#Dynamic_resizing
	try commenting out this loadfactor and make_bigger thing and running misc/mapping_speedtest.c
	java's 3/4 seems to be a really good choice for this value, at least on my system
	*/
	float loadfactor = ((float) map->size) / map->nbuckets;
	if (loadfactor > 0.75) {
		int status = make_bigger(map);
		if (status != STATUS_OK)
			return status;
	}

	unsigned long i = keyhash % map->nbuckets;
	if (map->buckets[i]) {
		for (struct MappingItem *olditem = map->buckets[i]; olditem; olditem=olditem->next) {
			int cmpres = map->keycmp(olditem->key, key, userdata);
			assert(cmpres <= 1);
			if (cmpres < 0)   // error
				return cmpres;
			if (cmpres == 1) {
				// the keys are equal, update the value of the existing key
				olditem->value = value;
				return STATUS_OK;
			}
		}
	}

	// now we know that we need to add a new MappingItem
	struct MappingItem *item = malloc(sizeof(struct MappingItem));
	if (!item)
		return STATUS_NOMEM;
	item->keyhash = keyhash;
	item->key = key;
	item->value = value;
	item->next = NULL;

	// it's easier to insert this item before the old item
	// i learned this from k&r
	if (map->buckets[i])
		item->next = map->buckets[i];
	map->buckets[i] = item;
	map->size++;
	return STATUS_OK;
}


int mapping_get(struct Mapping *map, void *key, unsigned long keyhash, void **res, void *userdata)
{
	struct MappingItem *item = map->buckets[keyhash % map->nbuckets];
	if (item) {
		for ( ; item; item=item->next) {
			int cmpres = map->keycmp(item->key, key, userdata);
			assert(cmpres <= 1);
			if (cmpres < 0)   // error
				return cmpres;
			if (cmpres == 1) {   // the keys are equal
				*res = item->value;
				return 1;    // found
			}
		}
	}
	// empty bucket or nothing but hash collisions
	return 0;
}


int mapping_pop(struct Mapping *map, void *key, unsigned long keyhash, void **res, void *userdata)
{
	unsigned long i = keyhash % map->nbuckets;
	if (map->buckets[i]) {
		struct MappingItem *prev = NULL;
		for (struct MappingItem *item = map->buckets[i]; item; item=item->next) {
			int cmpres = map->keycmp(item->key, key, userdata);
			assert(cmpres <= 1);
			if (cmpres < 0)   // error
				return cmpres;
			if (cmpres == 1) {
				// found it
				if (res)
					*res = item->value;

				// if you modify this, keep in mind that item->next may be NULL
				if (prev)    // skip this item
					prev->next = item->next;
				else       // this is the first item of the bucket
					map->buckets[i] = item->next;
				free(item);
				map->size--;
				return 1;
			}
			prev = item;
		}
	}
	return 0;
}


static int make_bigger(struct Mapping *map)
{
	size_t newnbuckets = map->nbuckets*3;   // 50, 150, 450, ...
	if (newnbuckets < map->nbuckets)   // huge mapping, *3 overflowed
		return STATUS_NOMEM;

	struct MappingItem **tmp = realloc(map->buckets, newnbuckets * sizeof(struct MappingItem));
	if (!tmp)
		return STATUS_NOMEM;
	memset(tmp + map->nbuckets, 0, (newnbuckets - map->nbuckets)*sizeof(struct MappingItem));
	map->buckets = tmp;

	// all items that need to be moved go to the newly allocated space
	// not another location in the old space
	for (size_t oldi = 0; oldi < map->nbuckets /* old bucket count */; oldi++) {
		struct MappingItem *prev = NULL;
		struct MappingItem *next;
		for (struct MappingItem *item = map->buckets[oldi]; item; item=next) {
			next = item->next;   // must be before changing item->next

			size_t newi = item->keyhash % newnbuckets;
			if (oldi != newi) {   // move it
				assert(newi >= map->nbuckets /* old bucket count */);
				// skip the item in this bucket, same code as in mapping_pop()
				if (prev)
					prev->next = item->next;
				else
					map->buckets[oldi] = item->next;

				// put the item to the new bucket, same code as in mapping_set()
				if (map->buckets[newi])
					item->next = map->buckets[newi];
				map->buckets[newi] = item;
			}
			prev = item;
		}
	}

	map->nbuckets = newnbuckets;
	return STATUS_OK;
}


void mapping_clear(struct Mapping *map)
{
	for (size_t i=0; i < map->nbuckets; i++) {
		struct MappingItem *item = map->buckets[i];
		while (item) {
			struct MappingItem *tmp = item;
			item = item->next;
			free(tmp);   // must be after the ->next
		}
		map->buckets[i] = NULL;
	}
	map->size = 0;
}


void mapping_free(struct Mapping *map)
{
	mapping_clear(map);
	free(map->buckets);
	free(map);
}
