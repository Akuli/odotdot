// https://en.wikipedia.org/wiki/Hash_table

#include "hashtable.h"
#include <assert.h>
#include <limits.h>
#include <stdlib.h>
#include <string.h>
#include "common.h"

struct HashTable *hashtable_new(hashtable_cmpfunc keycmp)
{
	struct HashTable *ht = malloc(sizeof(struct HashTable));
	if(!ht)
		return NULL;
	ht->keycmp = keycmp;
	ht->size = 0;
	ht->nbuckets = 50;
	ht->buckets = calloc(ht->nbuckets, sizeof(struct HashTableItem));
	if(!ht->buckets) {
		free(ht);
		return NULL;
	}
	return ht;
}


static int make_bigger(struct HashTable *ht)
{
	assert(ht->nbuckets != UINT_MAX);   // the caller should check this

	unsigned int newnbuckets = (ht->nbuckets > UINT_MAX/3) ? UINT_MAX : ht->nbuckets*3;

	struct HashTableItem **newbuckets = calloc(newnbuckets, sizeof(struct HashTableItem));
	if (!newbuckets)
		return STATUS_NOMEM;

	for (size_t oldi = 0; oldi < ht->nbuckets; oldi++) {
		struct HashTableItem *next;
		for (struct HashTableItem *item = ht->buckets[oldi]; item; item=next) {
			next = item->next;   // must be before changing item->next

			// put the item to a new bucket, same code as in hashtable_set()
			size_t newi = item->keyhash % newnbuckets;
			item->next = newbuckets[newi];   // may be NULL
			newbuckets[newi] = item;
		}
	}

	free(ht->buckets);
	ht->buckets = newbuckets;
	ht->nbuckets = newnbuckets;
	return STATUS_OK;
}


int hashtable_set(struct HashTable *ht, void *key, unsigned int keyhash, void *value, void *userdata)
{
	/*
	https://en.wikipedia.org/wiki/Hash_table#Key_statistics
	https://en.wikipedia.org/wiki/Hash_table#Dynamic_resizing
	try commenting out this loadfactor and make_bigger thing and running misc/hashtable_speedtest.c
	java's 3/4 seems to be a really good choice for this value, at least on my system
	*/
	if (ht->nbuckets != UINT_MAX) {
		float loadfactor = ((float) ht->size) / ht->nbuckets;
		if (loadfactor > 0.75) {
			int status = make_bigger(ht);
			if (status != STATUS_OK)
				return status;
		}
	}

	unsigned int i = keyhash % ht->nbuckets;
	if (ht->buckets[i]) {
		for (struct HashTableItem *olditem = ht->buckets[i]; olditem; olditem=olditem->next) {
			int cmpres = ht->keycmp(olditem->key, key, userdata);
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

	// now we know that we need to add a new HashTableItem
	struct HashTableItem *item = malloc(sizeof(struct HashTableItem));
	if (!item)
		return STATUS_NOMEM;
	item->keyhash = keyhash;
	item->key = key;
	item->value = value;

	// it's easier to insert this item before the old item
	// i learned this from k&r
	item->next = ht->buckets[i];    // may be NULL
	ht->buckets[i] = item;
	ht->size++;
	return STATUS_OK;
}


int hashtable_get(struct HashTable *ht, void *key, unsigned int keyhash, void **res, void *userdata)
{
	for (struct HashTableItem *item = ht->buckets[keyhash % ht->nbuckets]; item; item=item->next) {
		int cmpres = ht->keycmp(item->key, key, userdata);
		assert(cmpres <= 1);
		if (cmpres < 0)   // error
			return cmpres;
		if (cmpres == 1) {   // the keys are equal
			*res = item->value;
			return 1;    // found
		}
	}
	// empty buckets or nothing but hash collisions
	return 0;
}


int hashtable_pop(struct HashTable *ht, void *key, unsigned int keyhash, void **res, void *userdata)
{
	unsigned int i = keyhash % ht->nbuckets;
	struct HashTableItem *prev = NULL;

	for (struct HashTableItem *item = ht->buckets[i]; item; item=item->next) {
		int cmpres = ht->keycmp(item->key, key, userdata);
		assert(cmpres <= 1);

		if (cmpres < 0)   // error
			return cmpres;
		if (cmpres == 1) {
			// found it
			if (res)
				*res = item->value;

			if (prev)    // skip this item
				prev->next = item->next;   // may be NULL
			else       // this is the first item of the bucket
				ht->buckets[i] = item->next;
			free(item);
			ht->size--;
			return 1;
		}
		prev = item;
	}
	return 0;
}


void hashtable_iterbegin(struct HashTable *ht, struct HashTableIterator *it)
{
	it->ht = ht;
	it->started = 0;
	it->lastbucketno = 0;
}

int hashtable_iternext(struct HashTableIterator *it)
{
	if (!(it->started)) {
		it->lastbucketno = 0;
		it->started = 1;
		goto starthere;     // OMG OMG ITS A GOTO KITTENS DIE OR SOMETHING
	}
	if (it->lastbucketno == it->ht->nbuckets)   // the end
		return 0;

	if (it->lastitem->next) {
		it->lastitem = it->lastitem->next;
	} else {
		it->lastbucketno++;
starthere:
		while (it->lastbucketno < it->ht->nbuckets && !(it->ht->buckets[it->lastbucketno]))
			it->lastbucketno++;
		if (it->lastbucketno == it->ht->nbuckets)   // the end
			return 0;
		it->lastitem = it->ht->buckets[it->lastbucketno];
	}

	it->key = it->lastitem->key;
	it->value = it->lastitem->value;
	return 1;
}


void hashtable_fclear(struct HashTable *ht, void (*f)(void*, void*, void*), void *data)
{
	for (size_t i=0; i < ht->nbuckets; i++) {
		struct HashTableItem *next;
		for (struct HashTableItem *item = ht->buckets[i]; item; item=next) {
			next = item->next;   // must be before destroying the item
			f(item->key, item->value, data);
			free(item);
		}
		ht->buckets[i] = NULL;
	}
	ht->size = 0;
}

static void lol(void *x, void *y, void *z) { }
void hashtable_clear(struct HashTable *ht) { hashtable_fclear(ht, lol, NULL); }


void hashtable_free(struct HashTable *ht)
{
	assert(ht->size==0);    // must clear first
	free(ht->buckets);
	free(ht);
}
