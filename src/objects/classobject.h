#ifndef OBJECTS_CLASSOBJECT_H
#define OBJECTS_CLASSOBJECT_H

#include <stdbool.h>
#include "../interpreter.h"         // IWYU pragma: keep
#include "../objectsystem.h"        // IWYU pragma: keep
#include "../unicode.h"             // IWYU pragma: keep
#include "function.h"

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

	// should create and return a new instance of the class
	// it's always possible to create a subclass of the class that doesn't call the setup method
	// so if not calling the setup method can cause the interpreter to segfault, use this instead of setup
	// args and opts are the arguments and options passed to the 'new' function, including the class
	// it's safe to assume that ARRAYOBJECT_LEN(args) >= 1 and the first arg is the class object
	// NULL means that object_new_noerr() is used instead
	functionobject_cfunc newinstance;
};

// creates a new class
// if newinstance is not given, it's taken from the baseclass
// if newinstance is given, a setup method that takes and ignores all args and opts is added
// RETURNS A NEW REFERENCE or NULL on error
struct Object *classobject_new(struct Interpreter *interp, char *name, struct Object *baseclass, bool inheritable, functionobject_cfunc newinstance);

// RETURNS A NEW REFERENCE or NULL on no mem, for builtins_setup() only
// newinstance is set to NULL
// if you use this for creating classes that have data, set newinstance later manually and create a setup() that ignores all args and opts
// this is because Object's setup makes sure that it's called with no args and no opts, but we use newinstance instead of overriding it
// doesn't set the name, see classobject_setname() and builtins_setup()
struct Object *classobject_new_noerr(struct Interpreter *interp, struct Object *baseclass, bool inheritable, functionobject_cfunc newinstance);

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
