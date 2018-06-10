#ifndef OBJECTS_CLASSOBJECT_H
#define OBJECTS_CLASSOBJECT_H

#include <stdbool.h>
#include "../interpreter.h"         // IWYU pragma: keep
#include "../objectsystem.h"        // IWYU pragma: keep
#include "../unicode.h"             // IWYU pragma: keep

typedef void (*classobject_foreachrefcb)(struct Object *ref, void *data);

// this is exposed for objectsystem.c
struct ClassObjectData {
	// not a String object because it makes creating the String class easier
	struct UnicodeString name;

	// Object's baseclass is set to NULL
	// other baseclasses can also be NULL when the class class and Object class
	// are being created
	struct Object *baseclass;

	// Mappings that implement attributes, keys are string objects, values are function objects
	// these are first set to NULL, and the NULLs are replaced with empty mappings on-the-fly when needed
	// it's kind of weird, but... RIGHT NOW i think its nice and simple
	struct Object *setters;   // setters are called with 2 arguments, instance and value, they return null
	struct Object *getters;   // getters are called with 1 argument, the instance

	// uninheritable classes break if they are inherited
	// for example, an Array subclass might forget to call Array's setup method
	// but array.c isn't prepared to that, so Array is not inheritable
	bool inheritable;

	// calls cb(ref, data) for each ref object that this object (obj) refers to
	// this is used for garbage collecting
	// can be NULL
	// use classobject_runforeachref() when calling these, it handles corner cases
	void (*foreachref)(struct Object *obj, void *data, classobject_foreachrefcb cb);
};

// creates a new class
// RETURNS A NEW REFERENCE or NULL on error
struct Object *classobject_new(struct Interpreter *interp, char *name, struct Object *baseclass, void (*foreachref)(struct Object*, void*, classobject_foreachrefcb), bool inheritable);

// RETURNS A NEW REFERENCE or NULL on no mem, for builtins_setup() only
struct Object *classobject_new_noerr(struct Interpreter *interp, char *name, struct Object *baseclass, void (*foreachref)(struct Object*, void*, classobject_foreachrefcb), bool inheritable);

// a nicer wrapper for object_new_noerr()
// RETURNS A NEW REFERENCE or NULL on error (but unlike object_new_noerr, it sets interp->err)
struct Object *classobject_newinstance(struct Interpreter *interp, struct Object *klass, void *data, void (*destructor)(struct Object*));

// just for builtins_setup()
// bad things happen if klass is not a class object
// sets interp->err and returns false on error
bool classobject_setname(struct Interpreter *interp, struct Object *klass, char *name);

// like obj->klass == klass, but checks for inheritance
// never fails if klass is a classobject, bad things happen if it isn't
// returns false on error
bool classobject_isinstanceof(struct Object *obj, struct Object *klass);

// use this instead of ((struct ClassobjectData *) obj->klass->data)->foreachref(obj, data, cb)
// ->foreachref may be NULL, this handles that and inheritance
void classobject_runforeachref(struct Object *obj, void *data, classobject_foreachrefcb cb);

// used only in builtins_setup(), RETURNS A NEW REFERENCE or NULL on no mem
// uses interp->builtins.Object
struct Object *classobject_create_Class_noerr(struct Interpreter *interp);

// returns false on error
bool classobject_addmethods(struct Interpreter *interp);

#endif    // OBJECTS_CLASSOBJECT_H
