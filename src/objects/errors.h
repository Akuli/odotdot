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

// if this fails, interp->err is set to an error describing that failure, so there's no need to check that
// that's why this thing returns void
// bad things happen if err is not an Error object
void errorobject_throw(struct Interpreter *interp, struct Object *err);

// the ultimate convenience function
// throws an error with a string created with stringobject_newfromfmt
void errorobject_throwfmt(struct Interpreter *interp, char *classname, char *fmt, ...);

// never fails
#define errorobject_setnomem(interp) do { \
		OBJECT_INCREF((interp), (interp)->builtins.nomemerr); \
		(interp)->err = (interp)->builtins.nomemerr; \
	} while (0)

#endif    // OBJECTS_ERRORS_H
