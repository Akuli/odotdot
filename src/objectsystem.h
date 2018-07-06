#ifndef OBJECTSYSTEM_H
#define OBJECTSYSTEM_H

#include <stdio.h>
#include <stdbool.h>
#include "interpreter.h"   // IWYU pragma: keep
#include "atomicincrdecr.h"


typedef void (*object_foreachrefcb)(struct Object *ref, void *cbdata);

// represents arbitrary data that may contain other objects
struct ObjectData {
	// can be anything
	void *data;

	// should run cb(refobject, cbdata) for each object that objdata holds a reference to
	// can be NULL
	void (*foreachref)(void *data, object_foreachrefcb cb, void *cbdata);

	// should free the data properly but NOT decref stuff that foreachref takes care of
	// this is called after decrefs with foreachref
	// can be NULL
	void (*destructor)(void *data);
};

// all ö objects are pointers to instances of this struct
struct Object {
	// see objects/classobject.h
	// may be NULL early in builtins_setup()
	struct Object *klass;

	// anything depending on the type of the object
	struct ObjectData objdata;

	// a Mapping where attribute setters and getters can store the values
	// this is NULL first, and created by attribute.c when needed
	// exposed only for builtins.ö
	struct Object *attrdata;

	// if hashable is 1, this object can be used as a key in Mappings
	bool hashable;
	long hash;

	// use with atomicincrdecr.h functions only
	long refcount;

	// the garbage collector does something implementation-detaily with this
	long gcflag;
};

void lol(void);void wat(void);


// low-level function for creating a new object, see also classobject_newinstance()
// create a new object, add it to interp->allobjects and return it, returns NULL on no mem
// shouldn't be used with classes that have a newinstance, except when implementing newinstance
// see above for descriptions of data and destructor
// RETURNS A NEW REFERENCE, i.e. refcount is set to 1
struct Object *object_new_noerr(struct Interpreter *interp, struct Object *klass, struct ObjectData objdata);

// these never fail
#define OBJECT_INCREF(interp, obj) do { \
	if ((obj) == (interp)->builtins.Class) \
		lol(); \
	ATOMIC_INCR((obj)->refcount); \
} while(0)

#define OBJECT_DECREF(interp, obj) do { \
	if ((obj) == (interp)->builtins.Class) \
		wat(); \
	if (ATOMIC_DECR((obj)->refcount) <= 0) \
		object_free_impl((interp), (obj), false); \
} while (0)

// like the name says, this is pretty much an implementation detail
// it's here just because OBJECT_{IN,DE}CREF need it
// use OBJECT_DECREF instead of calling this
// you may need to modify gc_run() if you modify this
void object_free_impl(struct Interpreter *interp, struct Object *obj, bool calledfromgc);


#endif   // OBJECTSYSTEM_H
