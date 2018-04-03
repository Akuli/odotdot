#ifndef MAPPING_H
#define MAPPING_H

#include <stddef.h>

// should return 1 for a equals b, 0 for a does not equal b or something negative for error
// must not return 2 or greater
typedef int (*hashtable_cmpfunc)(void *a, void *b, void *userdata);

// should be considered an implementation detail
struct HashTableItem {
	unsigned int keyhash;
	void *key;
	void *value;
	struct HashTableItem *next;
};

// everything except size should be considered implementation details
struct HashTable {
	struct HashTableItem **buckets;
	size_t nbuckets;
	size_t size;
	hashtable_cmpfunc keycmp;
};

// returns NULL on no mem
struct HashTable *hashtable_new(hashtable_cmpfunc keycmp);

// return STATUS_OK on success, and on failure STATUS_NOMEM or an error code from cmpfunc
int hashtable_set(struct HashTable *ht, void *key, unsigned int keyhash, void *value, void *userdata);

// return 1 if key was found, 0 if not (*res is not set) or an error code from cmpfunc
int hashtable_get(struct HashTable *ht, void *key, unsigned int keyhash, void **res, void *userdata);

// delete an item from the hashtable and set it to *res, or ignore it if res is NULL
// returns 1 if the key was found, 0 if not and an error code from cmpfunc on failure
int hashtable_pop(struct HashTable *ht, void *key, unsigned int keyhash, void **res, void *userdata);

// everything except key and value should be considered implementation details
struct HashTableIterator {
	void *key;
	void *value;

	struct HashTable *ht;
	int started;
	size_t lastbucketno;
	struct HashTableItem *lastitem;
};

/* usage example:

	struct HashTableIterator iter;
	hashtable_iterbegin(ht, &iter);
	while (hashtable_iternext(&iter)) {
		// do something with iter.key and iter.value
	}

these functions never fail
*/
void hashtable_iterbegin(struct HashTable *ht, struct HashTableIterator *it);
int hashtable_iternext(struct HashTableIterator *it);     // always returns 1 or 0

// pop all keys, never fails
void hashtable_clear(struct HashTable *ht);

// like hashtable_clear(), but calls f(key, value, data) for every key,value pair in the hashtable
void hashtable_fclear(struct HashTable *ht, void (*f)(void*, void*, void*), void *data);

// deallocate a hashtable, must be cleared first
void hashtable_free(struct HashTable *ht);

//int hashtable_contains(struct HashTable *ht, void *key, unsigned int keyhash, void *userdata);
//int hashtable_equals(struct HashTable *ht1, struct HashTable *ht2, hashtable_cmpfunc valuecmp, void *userdata);

//unsigned int hashtable_stringhash(unicode_t *string, size_t stringlen);

#endif   // MAPPING_H
