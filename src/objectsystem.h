#ifndef OBJECTSYSTEM_H
#define OBJECTSYSTEM_H

#include "hashtable.h"     // IWYU pragma: keep
#include "interpreter.h"   // IWYU pragma: keep
#include "unicode.h"       // IWYU pragma: keep
#include "atomicincrdecr.h"


// iwyu is stupid, why can't i pragma keep structs forward declarations :(
struct Object;   // forward declaration

typedef void (*objectclassinfo_foreachrefcb)(struct Object *ref, void *data);
typedef void (*objectclassinfo_foreachref)(struct Object *obj, void *data, objectclassinfo_foreachrefcb cb);

/* every ö class is represented as an ObjectClassInfo struct wrapped in a classobject
   see objects/classobject.h */
struct ObjectClassInfo {
	// TODO: something better
	char name[10];

	// Object's baseclass is set to NULL
	// FIXME: class objects should hold references to base classes
	// TODO: should this be a class object?
	struct ObjectClassInfo *baseclass;

	// keys are UnicodeStrings, values are Function objects (see objects/function.{c,h})
	struct HashTable *methods;

	// calls cb(ref, data) for each ref object that this object refers to
	// this is used for garbage collecting
	// can be NULL
	objectclassinfo_foreachref foreachref;

	// called by object_free_impl (see OBJECT_DECREF)
	// can be NULL
	void (*destructor)(struct Object *obj);
};

// all ö objects are instances of this struct
struct Object {
	// see objects/classobject.h
	struct Object *klass;

	// keys are UnicodeStrings pointers, values are pointers to Function objects
	struct HashTable *attrs;

	// NULL for objects created from the language, but constructor functions
	// written in C can use this for anything
	void *data;

	// use with atomicincrdecr.h functions only
	int refcount;

	// the garbage collector does something implementation-detaily with this
	int gcflag;
};

// helper for blabla_createclass() functions, returns NULL on no mem
struct ObjectClassInfo *objectclassinfo_new(char *name, struct ObjectClassInfo *base, objectclassinfo_foreachref foreachref, void (*destructor)(struct Object *));

// never fails
void objectclassinfo_free(struct Interpreter *interp, struct ObjectClassInfo *klass);

// create a new object, add it to interp->allobjects and return it, returns NULL on no mem
// see also classobject_newinstance() in objects/classobject.h
// caller MUST hold a reference to the klass
// RETURNS A NEW REFERENCE
struct Object *object_new(struct Interpreter *interp, struct Object *klass, void *data);

// quite self-explanatory, these never fail
#define OBJECT_INCREF(interp, obj) do { ATOMIC_INCR(((struct Object *)(obj))->refcount); } while(0)
#define OBJECT_DECREF(interp, obj) do { \
	if (ATOMIC_DECR(((struct Object *)(obj))->refcount) <= 0) \
		object_free_impl((interp), (obj)); \
} while (0)

// like the name says, this is pretty much an implementation detail
// calls obj->klass->destructor and frees memory allocated by obj
// this removes obj from interp->allobjects
// use OBJECT_DECREF instead of calling this
void object_free_impl(struct Interpreter *interp, struct Object *obj);


#endif   // OBJECTSYSTEM_H
