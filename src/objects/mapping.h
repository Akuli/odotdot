#ifndef OBJECTS_MAPPING_H
#define OBJECTS_MAPPING_H

#include <stdbool.h>
#include <stddef.h>
#include "../interpreter.h"     // IWYU pragma: keep
#include "../objectsystem.h"    // IWYU pragma: keep

// these should be considered implementation details
// stupid IWYU hates the MappingObjectItem declaration and has no way to ignore it
struct MappingObjectItem;
struct MappingObjectData {
	struct MappingObjectItem **buckets;
	// hashes are longs, so more than ULONG_MAX buckets would be useless
	// long and unsigned long are of the same size
	unsigned long nbuckets;
	size_t size;
};


// RETURNS A NEW REFERENCE or NULL on error
struct Object *mappingobject_createclass(struct Interpreter *interp);

// returns false on error
// methods are stored in a Mapping, so this can't be a part of mappingobject_createclass()
bool mappingobject_addmethods(struct Interpreter *interp);

// for builtins_setup()
bool mappingobject_initoparrays(struct Interpreter *interp);

// RETURNS A NEW REFERENCE or NULL on error
struct Object *mappingobject_newempty(struct Interpreter *interp);

// returns number of elements in the mapping
#define MAPPINGOBJECT_SIZE(map) (((struct MappingObjectData *) (map)->objdata.data)->size)

// returns false on error
// bad things happen if map is not a Mapping
bool mappingobject_set(struct Interpreter *interp, struct Object *map, struct Object *key, struct Object *val);

/* return values:
0	key not found, *val is unchanged
1	key found, a new reference has been set to *val
-1	an error occurred, *val is unchanged, interp->err was set

bad things happen if map is not a Mapping
*/
int mappingobject_get(struct Interpreter *interp, struct Object *map, struct Object *key, struct Object **val);

// like mappingobject_get, but deletes the key from the mapping as well
int mappingobject_getanddelete(struct Interpreter *interp, struct Object *map, struct Object *key, struct Object **val);


struct MappingObjectIter {
	struct Object *key;
	struct Object *value;

	// rest of these should be considered implementation details
	struct MappingObjectData *data;
	bool started;
	unsigned long lastbucketno;
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
bool mappingobject_iternext(struct MappingObjectIter *it);

#endif   // OBJECTS_MAPPING_H
