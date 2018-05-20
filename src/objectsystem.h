#ifndef OBJECTSYSTEM_H
#define OBJECTSYSTEM_H

#include <stdbool.h>
#include "interpreter.h"   // IWYU pragma: keep
#include "atomicincrdecr.h"


// all ö objects are instances of this struct
struct Object {
	// see objects/classobject.h
	struct Object *klass;

	// a Mapping object or NULL, keys are String objects
	struct Object *attrs;

	// NULL for objects created from the language, but constructor functions
	// written in C can use this for anything
	void *data;

	// a function that cleans up obj->data, can be NULL
	void (*destructor)(struct Object *obj);

	// if hashable is 1, this object can be used as a key in Mappings
	int hashable;    // 1 or 0
	int hash;

	// use with atomicincrdecr.h functions only
	int refcount;

	// the garbage collector does something implementation-detaily with this
	int gcflag;
};

// create a new object, add it to interp->allobjects and return it, returns NULL on no mem
// if you think you want to call this, you probably want classobject_newinstance() instead
// CANNOT BE USED with classes that want their instances to have attributes, use classobject_newinstance()
// see above for descriptions of data and destructor
// hashable is set to 1, and hash is set to a value based on (uintptr_t)returned_obj
// RETURNS A NEW REFERENCE, i.e. refcount is set to 1
struct Object *object_new(struct Interpreter *interp, struct Object *klass, void *data, void (*destructor)(struct Object*));

// quite self-explanatory, these never fail
#define OBJECT_INCREF(interp, obj) do { ATOMIC_INCR(((struct Object *)(obj))->refcount); } while(0)
#define OBJECT_DECREF(interp, obj) do { \
	if (ATOMIC_DECR(((struct Object *)(obj))->refcount) <= 0) \
		object_free_impl((interp), (obj), true); \
} while (0)

// like the name says, this is pretty much an implementation detail
// use OBJECT_DECREF instead of calling this
// calls obj->destructor and frees memory allocated by obj, optionally removes obj from interp->allobjects
// you may need to modify gc_run() if you modify this
void object_free_impl(struct Interpreter *interp, struct Object *obj, bool removefromallobjects);


#endif   // OBJECTSYSTEM_H
