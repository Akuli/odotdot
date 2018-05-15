#ifndef OBJECTSYSTEM_H
#define OBJECTSYSTEM_H

#include "hashtable.h"     // IWYU pragma: keep
#include "interpreter.h"   // IWYU pragma: keep
#include "atomicincrdecr.h"


// all ö objects are instances of this struct
struct Object {
	// see objects/classobject.h
	struct Object *klass;

	// keys are UnicodeStrings pointers, values are pointers to objects
	struct HashTable *attrs;

	// NULL for objects created from the language, but constructor functions
	// written in C can use this for anything
	void *data;

	// use with atomicincrdecr.h functions only
	int refcount;

	// the garbage collector does something implementation-detaily with this
	int gcflag;
};

// create a new object, add it to interp->allobjects and return it, returns NULL on no mem
// see also classobject_newinstance() in objects/classobject.h
// RETURNS A NEW REFERENCE
struct Object *object_new(struct Interpreter *interp, struct Object *klass, void *data);

// quite self-explanatory, these never fail
#define OBJECT_INCREF(interp, obj) do { ATOMIC_INCR(((struct Object *)(obj))->refcount); } while(0)
#define OBJECT_DECREF(interp, obj) do { \
	if (ATOMIC_DECR(((struct Object *)(obj))->refcount) <= 0) \
		object_free_impl((interp), (obj), 1); \
} while (0)

// like the name says, this is pretty much an implementation detail
// calls obj->klass->destructor and frees memory allocated by obj
// gc.c calls this with decref_refs=0 when everything is about to be freed
// this removes obj from interp->allobjects
// use OBJECT_DECREF instead of calling this
void object_free_impl(struct Interpreter *interp, struct Object *obj, int decref_refs);


#endif   // OBJECTSYSTEM_H
