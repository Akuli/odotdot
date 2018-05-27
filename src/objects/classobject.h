#ifndef OBJECTS_CLASSOBJECT_H
#define OBJECTS_CLASSOBJECT_H

#include "../interpreter.h"         // IWYU pragma: keep
#include "../objectsystem.h"        // IWYU pragma: keep

typedef void (*classobject_foreachrefcb)(struct Object *ref, void *data);

// this is exposed for objectsystem.c
struct ClassObjectData {
	// TODO: something better
	char name[10];

	// Object's baseclass is set to NULL
	// other baseclasses can also be NULL when the class class and Object class
	// are being created
	struct Object *baseclass;

	// a Mapping, or NULL if the class was created before Mappings existed
	struct Object *methods;

	// instances of e.g. String and Integer have no attributes
	int instanceshaveattrs;   // 1 or 0

	// calls cb(ref, data) for each ref object that this object (obj) refers to
	// this is used for garbage collecting
	// can be NULL
	// use classobject_runforeachref() when calling these, it handles corner cases
	void (*foreachref)(struct Object *obj, void *data, classobject_foreachrefcb cb);
};

// creates a new class
// RETURNS A NEW REFERENCE or NULL on error
struct Object *classobject_new(struct Interpreter *interp, char *name, struct Object *base, int instanceshaveattrs, void (*foreachref)(struct Object*, void*, classobject_foreachrefcb));

// RETURNS A NEW REFERENCE or NULL on no mem, for builtins_setup() only
struct Object *classobject_new_noerr(struct Interpreter *interp, char *name, struct Object *base, int instanceshaveattrs, void (*foreachref)(struct Object*, void*, classobject_foreachrefcb));

// a nicer wrapper for object_new_noerr()
// RETURNS A NEW REFERENCE or NULL on error (but unlike object_new_noerr, it sets interp->err)
struct Object *classobject_newinstance(struct Interpreter *interp, struct Object *klass, void *data, void (*destructor)(struct Object*));

// like obj->klass == klass, but checks for inheritance
// never fails if klass is a classobject, bad things happen if it isn't
// returns 1 or 0
int classobject_instanceof(struct Object *obj, struct Object *klass);

// use this instead of ((struct ClassobjectData *) obj->klass->data)->foreachref(obj, data, cb)
// ->foreachref may be NULL, this handles that and inheritance
void classobject_runforeachref(struct Object *obj, void *data, classobject_foreachrefcb cb);

// used only in builtins_setup(), RETURNS A NEW REFERENCE or NULL on no mem
// uses interp->builtins.objectclass
struct Object *classobject_create_classclass_noerr(struct Interpreter *interp);

// returns STATUS_OK or STATUS_ERROR
int classobject_addmethods(struct Interpreter *interp);

#endif    // OBJECTS_CLASSOBJECT_H
