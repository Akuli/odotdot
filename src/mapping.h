// hashmap implementation
#ifndef MAPPING_H
#define MAPPING_H

#include <stddef.h>

// should return 1 for a equals b, 0 for a does not equal b or something negative for error
// must not return 2 or greater
typedef int (*mapping_cmpfunc)(void *a, void *b, void *userdata);

// should be considered an implementation detail
struct MappingItem {
	unsigned long keyhash;    // longs might be useful for a huuuuuge hashmap
	void *key;
	void *value;
	struct MappingItem *next;
};

// everything except size should be considered implementation details
struct Mapping {
	struct MappingItem **buckets;
	size_t nbuckets;
	size_t size;
	mapping_cmpfunc keycmp;
};

struct Mapping *mapping_new(mapping_cmpfunc keycmp);

// return STATUS_OK on success, and on failure STATUS_NOMEM or an error code from cmpfunc
int mapping_set(struct Mapping *map, void *key, unsigned long keyhash, void *value, void *userdata);

// return 1 if key was found, 0 if not (*res is not set) or an error code from cmpfunc
int mapping_get(struct Mapping *map, void *key, unsigned long keyhash, void **res, void *userdata);

// delete an item from the mapping and set it to *res, or ignore it if res is NULL
// returns 1 if the key was found, 0 if not and an error code from cmpfunc on failure
int mapping_pop(struct Mapping *map, void *key, unsigned long keyhash, void **res, void *userdata);

// pop all keys, never fails
void mapping_clear(struct Mapping *map);

// deallocate a mapping
void mapping_free(struct Mapping *map);

//int mapping_contains(struct Mapping *map, void *key, unsigned long keyhash, void *userdata);
//int mapping_equals(struct Mapping *map1, struct Mapping *map2, mapping_cmpfunc valuecmp, void *userdata);

//unsigned long mapping_stringhash(unicode_t *string, size_t stringlen);

#endif   // MAPPING_H
