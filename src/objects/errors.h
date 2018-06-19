#ifndef OBJECTS_ERRORS_H
#define OBJECTS_ERRORS_H

#include "../interpreter.h"     // IWYU pragma: keep
#include "../objectsystem.h"    // IWYU pragma: keep

// returns NULL on no mem
struct Object *errorobject_createclass_noerr(struct Interpreter *interp);

// returns false on error
bool errorobject_addmethods(struct Interpreter *interp);

// RETURNS A NEW REFERENCE or NULL on no mem
struct Object *errorobject_createnomemerr_noerr(struct Interpreter *interp);

// RETURNS A NEW REFERENCE or NULL on error
struct Object *errorobject_createmarkererrorclass(struct Interpreter *interp);

// NOT GUARANTEED TO WORK with all error classes
// but all built-in errors work fine
// RETURNS A NEW REFERENCE or NULL on error
struct Object *errorobject_new(struct Interpreter *interp, struct Object *errorclass, struct Object *messagestring);

// if this fails, interp->err is set to an error describing that failure, so there's no need to check that
// that's why this thing returns void
// bad things happen if err is not an Error object
void errorobject_throw(struct Interpreter *interp, struct Object *err);

// the ultimate convenience function
// throws an error with a string created with stringobject_newfromfmt
void errorobject_throwfmt(struct Interpreter *interp, char *classname, char *fmt, ...);

// prints "ClassName: message" and a stack trace to stderr
// if builtins.ö hasn't been loaded, there will be no stack trace
// if this fails, the failure is printed
// interp->err must be NULL when calling this, and it's always NULL when this returns
void errorobject_print(struct Interpreter *interp, struct Object *err);

// never fails
// FIXME: doesn't set stack
#define errorobject_thrownomem(interp) do { \
		OBJECT_INCREF((interp), (interp)->builtins.nomemerr); \
		(interp)->err = (interp)->builtins.nomemerr; \
	} while (0)

#endif    // OBJECTS_ERRORS_H
