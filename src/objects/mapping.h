#ifndef OBJECTS_MAPPING_H
#define OBJECTS_MAPPING_H

#include <stddef.h>
#include "../interpreter.h"     // IWYU pragma: keep
#include "../objectsystem.h"    // IWYU pragma: keep

// should be considered an implementation detail
struct MappingObjectItem;

// everything except size should be considered implementation details
struct MappingObjectData {
	struct MappingObjectItem **buckets;
	unsigned int nbuckets;   // hashes are uints, so more than UINT_MAX buckets would be useless
	size_t size;
};


// RETURNS A NEW REFERENCE or NULL on error
struct Object *mappingobject_createclass(struct Interpreter *interp, struct Object **errptr);

// returns STATUS_OK or STATUS_ERROR
// methods are stored in a Mapping, so this can't be a part of mappingobject_createclass()
int mappingobject_addmethods(struct Interpreter *interp, struct Object **errptr);

// RETURNS A NEW REFERENCE or NULL on error
struct Object *mappingobject_newempty(struct Interpreter *interp, struct Object **errptr);

// returns STATUS_OK or STATUS_ERROR
// bad things happen if map is not a Mapping
int mappingobject_set(struct Interpreter *interp, struct Object **errptr, struct Object *map, struct Object *key, struct Object *val);

// RETURNS A NEW REFERENCE or NULL on error
// if the key is not found, RETURNS NULL WITHOUT SETTING ERRPTR
// bad things happen if map is not a Mapping
struct Object *mappingobject_get(struct Interpreter *interp, struct Object **errptr, struct Object *map, struct Object *key);


struct MappingObjectIter {
	struct Object *key;
	struct Object *value;

	// rest of these should be considered implementation details
	struct Interpreter *interp;
	struct Object **errptr;
	struct MappingObjectData *data;
	int started;
	unsigned int lastbucketno;
	struct MappingObjectItem *lastitem;
};

/* usage:

	struct MappingObjectIter iter;
	mappingobject_iterbegin(&iter, map);
	while (mappingobject_iternext(&iter)) {
		// do something with iter.key and iter.value
		// the mapping must not be modified here!
	}

bad things happen if map is not a Mapping object
otherwise these functions never fail
*/

void mappingobject_iterbegin(struct MappingObjectIter *it, struct Object *map);
int mappingobject_iternext(struct MappingObjectIter *it);   // always returns 1 or 0

#endif   // OBJECTS_MAPPING_H
