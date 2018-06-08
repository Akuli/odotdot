// see objects/mapping.c for more comments and stuff
#include "allobjects.h"
#include <assert.h>
#include <limits.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>    // TODO: support systems without uintptr_t?
#include <stdlib.h>

struct AllObjectsItem {
	struct Object *obj;
	struct AllObjectsItem *next;
};

bool allobjects_init(struct AllObjects *ao)
{
	ao->nbuckets = 100;
	ao->buckets = calloc(ao->nbuckets, sizeof(struct AllObjectsItem));
	if (!ao->buckets)
		return false;
	ao->size = 0;
	return true;
}

void allobjects_free(struct AllObjects ao)
{
	for (size_t i=0; i < ao.nbuckets; i++) {
		struct AllObjectsItem *item = ao.buckets[i];
		while (item) {
			void *gonnafree = item;
			item = item->next;   // must be before the free
			free(gonnafree);
		}
	}
	free(ao.buckets);
}


static unsigned long object_hash(struct Object *obj)
{
	// same as the default hash in objectsystem.c
	return (uintptr_t)((void*)obj) >> 2;
}

static bool make_bigger(struct AllObjects *ao)
{
	assert(ao->nbuckets != ULONG_MAX);   // the caller should check this
	unsigned long newnbuckets = (ao->nbuckets > ULONG_MAX/2) ? ULONG_MAX : ao->nbuckets*2;

	struct AllObjectsItem **newbuckets = calloc(newnbuckets, sizeof(struct AllObjectsItem));
	if (!newbuckets)
		return false;

	for (size_t oldi = 0; oldi < ao->nbuckets; oldi++) {
		struct AllObjectsItem *next;
		for (struct AllObjectsItem *item = ao->buckets[oldi]; item; item=next) {
			next = item->next;   // must be before changing item->next
			size_t newi = object_hash(item->obj) % newnbuckets;
			item->next = newbuckets[newi];   // may be NULL
			newbuckets[newi] = item;
		}
	}

	free(ao->buckets);
	ao->buckets = newbuckets;
	ao->nbuckets = newnbuckets;
	return true;
}

bool allobjects_add(struct AllObjects *ao, struct Object *obj)
{
	if (ao->nbuckets != ULONG_MAX) {
		float loadfactor = ((float) ao->size) / ao->nbuckets;
		if (loadfactor > 0.75) {
			if (!make_bigger(ao))
				return false;
		}
	}

	struct AllObjectsItem *item = malloc(sizeof(struct AllObjectsItem));
	if (!item)
		return false;
	item->obj = obj;

	unsigned long i = object_hash(obj) % ao->nbuckets;
	item->next = ao->buckets[i];    // may be NULL
	ao->buckets[i] = item;
	ao->size++;
	return true;
}

bool allobjects_remove(struct AllObjects *ao, struct Object *obj)
{
	unsigned int i = object_hash(obj) % ao->nbuckets;
	struct AllObjectsItem *prev = NULL;

	for (struct AllObjectsItem *item = ao->buckets[i]; item; item=item->next) {
		if (item->obj == obj) {
			// if you modify this, keep in mind that item->next may be NULL
			if (prev)   // skip this item
				prev->next = item->next;
			else      // this is the first item of the bucket
				ao->buckets[i] = item->next;
			ao->size--;
			free(item);
			return true;
		}
		prev = item;
	}
	return false;
}


struct AllObjectsIter allobjects_iterbegin(struct AllObjects	ao)
{
	struct AllObjectsIter it;
	it.ao = ao;
	it.started = false;
	it.lastbucketno = 0;
	return it;
}

bool allobjects_iternext(struct AllObjectsIter *it)
{
	if (!it->started) {
		it->lastbucketno = 0;
		it->started = true;
		goto starthere;
	}
	if (it->lastbucketno == it->ao.nbuckets)
		return false;

	if (it->lastitem->next) {
		it->lastitem = it->lastitem->next;
	} else {
		it->lastbucketno++;
starthere:
		while (it->lastbucketno < it->ao.nbuckets && !(it->ao.buckets[it->lastbucketno]))
			it->lastbucketno++;
		if (it->lastbucketno == it->ao.nbuckets)
			return false;
		it->lastitem = it->ao.buckets[it->lastbucketno];
	}

	it->obj = it->lastitem->obj;
	return true;
}
