#ifndef OBJECTS_CLASSOBJECT_H
#define OBJECTS_CLASSOBJECT_H

#include "../interpreter.h"         // IWYU pragma: keep
#include "../objectsystem.h"

typedef void (*classobject_foreachrefcb)(struct Object *ref, void *data);

// this is exposed for objectsystem.c
struct ClassObjectData {
	// TODO: something better
	char name[10];

	// Object's baseclass is set to NULL
	// other baseclasses can also be NULL when the class class and Object class
	// are being created
	struct Object *baseclass;

	// keys are UnicodeStrings, values are Function objects (see objects/function.{c,h})
	struct HashTable *methods;

	// calls cb(ref, data) for each ref object that this object refers to
	// this is used for garbage collecting
	// can be NULL
	void (*foreachref)(struct Object *obj, void *data, classobject_foreachrefcb cb);

	// called by object_free_impl (see OBJECT_DECREF)
	// can be NULL
	void (*destructor)(struct Object *obj);
};

// the destructor that classes use, exposed for builtins_teardown()
void classobject_destructor(struct Object *klass);

// creates a new class
// RETURNS A NEW REFERENCE or NULL on error
struct Object *classobject_new(struct Interpreter *interp, struct Object **errptr, char *name, struct Object *base, void (*foreachref)(struct Object*, void*, classobject_foreachrefcb), void (*destructor)(struct Object*));

// RETURNS A NEW REFERENCE or NULL on no mem, for builtins_setup() only
struct Object *classobject_new_noerrptr(struct Interpreter *interp, char *name, struct Object *base, void (*foreachref)(struct Object*, void*, classobject_foreachrefcb), void (*destructor)(struct Object*));

// a nicer wrapper for object_new() that takes errptr and checks types
// RETURNS A NEW REFERENCE or NULL on error
struct Object *classobject_newinstance(struct Interpreter *interp, struct Object **errptr, struct Object *klass, void *data);

// like obj->klass == klass, but checks for inheritance
// never fails if klass is a classobject, bad things happen if it isn't
// returns 1 or 0
int classobject_instanceof(struct Object *obj, struct Object *klass);

// used only in builtins_setup(), RETURNS A NEW REFERENCE or NULL on no mem
struct Object *classobject_create_classclass(struct Interpreter *interp, struct Object *objectclass);

#endif    // OBJECTS_CLASSOBJECT_H
