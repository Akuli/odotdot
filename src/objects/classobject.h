#ifndef OBJECTS_CLASSOBJECT_H
#define OBJECTS_CLASSOBJECT_H

#include <stdbool.h>
#include "../interpreter.h"         // IWYU pragma: keep
#include "../objectsystem.h"        // IWYU pragma: keep
#include "../unicode.h"             // IWYU pragma: keep

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
	// TODO: get rid of this, other people hate it
	bool inheritable;
};

// creates a new class
// RETURNS A NEW REFERENCE or NULL on error
struct Object *classobject_new(struct Interpreter *interp, char *name, struct Object *baseclass, bool inheritable);

// RETURNS A NEW REFERENCE or NULL on no mem, for builtins_setup() only
// doesn't set the name, see classobject_setname() and builtins_setup()
struct Object *classobject_new_noerr(struct Interpreter *interp, struct Object *baseclass, bool inheritable);

// a nicer wrapper for object_new_noerr()
// RETURNS A NEW REFERENCE or NULL on error (but unlike object_new_noerr, it sets interp->err)
struct Object *classobject_newinstance(struct Interpreter *interp, struct Object *klass,
	void *data, void (*foreachref)(struct Object *, void *, object_foreachrefcb), void (*destructor)(struct Object *));

// just for builtins_setup()
// bad things happen if klass is not a class object
// sets interp->err and returns false on error
bool classobject_setname(struct Interpreter *interp, struct Object *klass, char *name);

// checks if instances of sub are instances of super
// like sub == super, but checks for inheritance
// bad things happen if sub or super is not a class object, otherwise never fails
bool classobject_issubclassof(struct Object *sub, struct Object *super);

// convenience ftw
#define classobject_isinstanceof(obj, objklass) classobject_issubclassof((obj)->klass, (objklass))

// used only in builtins_setup(), RETURNS A NEW REFERENCE or NULL on no mem
// uses interp->builtins.Object
struct Object *classobject_create_Class_noerr(struct Interpreter *interp);

// returns false on error
bool classobject_addmethods(struct Interpreter *interp);

#endif    // OBJECTS_CLASSOBJECT_H
