#ifndef OBJECTSYSTEM_H
#define OBJECTSYSTEM_H

#include <stdio.h>
#include <stdbool.h>
#include "interpreter.h"   // IWYU pragma: keep
#include "atomicincrdecr.h"


// all ö objects are pointers to instances of this struct
struct Object {
	// see objects/classobject.h
	struct Object *klass;

	// can be anything, depends on the type of the object
	void *data;

	// a Mapping where attribute setters and getters can store the values
	// this is NULL first, and created by attribute.c when needed
	// not exposed in the language
	struct Object *attrdata;

	// a function that cleans up obj->data, can be NULL
	void (*destructor)(struct Object *obj);

	// if hashable is 1, this object can be used as a key in Mappings
	bool hashable;
	long hash;

	// use with atomicincrdecr.h functions only
	long refcount;

	// the garbage collector does something implementation-detaily with this
	long gcflag;
};

// THIS IS LOWLEVEL, see classobject_newinstance()
// create a new object, add it to interp->allobjects and return it, returns NULL on no mem
// CANNOT BE USED with classes that want their instances to have attributes
// see above for descriptions of data and destructor
// RETURNS A NEW REFERENCE, i.e. refcount is set to 1
struct Object *object_new_noerr(struct Interpreter *interp, struct Object *klass, void *data, void (*destructor)(struct Object*));

// these never fail
#define OBJECT_INCREF(interp, obj) do { ATOMIC_INCR(((struct Object *)(obj))->refcount); } while(0)
#define OBJECT_DECREF(interp, obj) do { \
	if (ATOMIC_DECR(((struct Object *)(obj))->refcount) <= 0) \
		object_free_impl((interp), (obj), true); \
} while (0)

// like the name says, this is pretty much an implementation detail
// it's here just because OBJECT_{INCREF,DECREF} need it
// use OBJECT_DECREF instead of calling this
// calls obj->destructor and frees memory allocated by obj, optionally removes obj from interp->allobjects
// you may need to modify gc_run() if you modify this
void object_free_impl(struct Interpreter *interp, struct Object *obj, bool removefromallobjects);


#endif   // OBJECTSYSTEM_H
