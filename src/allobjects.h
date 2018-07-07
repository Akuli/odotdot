// a hashtable-like thing with no values for keeping track of all objects in the interpreter
#ifndef ALLOBJECTS_H
#define ALLOBJECTS_H

#include <stdbool.h>
#include <stddef.h>

// stupid IWYU doesn't get these
struct Object;
struct AllObjectsItem;   // should be considered an implementation detail

struct AllObjects {
	// everything except size should be considered implementation details
	struct AllObjectsItem **buckets;
	unsigned long nbuckets;
	size_t size;
};

// returns true on success and false on no mem
bool allobjects_init(struct AllObjects *ao);

// never fails
void allobjects_free(struct AllObjects ao);

// returns true on success and false on no mem
bool allobjects_add(struct AllObjects *ao, struct Object *obj);

// returns true if the object was found and false if it wasn't
bool allobjects_remove(struct AllObjects *ao, struct Object *obj);

/* usage:

	struct AllObjectsIter iter = allobjects_iterbegin(ao);
	while (allobjects_iternext(&iter)) {
		// do something with iter.obj
	}
*/
struct AllObjectsIter {
	struct Object *obj;

	// rest of these should be considered implementation details
	struct AllObjects ao;
	bool started;
	unsigned long lastbucketno;
	struct AllObjectsItem *lastitem;
};
struct AllObjectsIter allobjects_iterbegin(struct AllObjects	ao);
bool allobjects_iternext(struct AllObjectsIter *it);

#endif    // ALLOBJECTS_H
