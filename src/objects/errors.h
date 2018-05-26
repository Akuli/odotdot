// TODO: add subclasses of Error
#ifndef OBJECTS_ERRORS_H
#define OBJECTS_ERRORS_H

#include "../interpreter.h"     // IWYU pragma: keep
#include "../objectsystem.h"    // IWYU pragma: keep

// returns NULL on no mem
struct Object *errorobject_createclass_noerr(struct Interpreter *interp);

// returns STATUS_OK or STATUS_ERROR
int errorobject_addmethods(struct Interpreter *interp);

// RETURNS A NEW REFERENCE or NULL on no mem
struct Object *errorobject_createnomemerr_noerr(struct Interpreter *interp);

// the ultimate convenience function
// sets interp->err to an error with a string created with stringobject_newfromfmt
// it was named errorobject_seterrfromfmtstring but that was way too long to type
// i thought about errorobject_sethandy, but that wouldn't be very descriptive
// on failure, sets interp->err to some other error and returns STATUS_ERROR, otherwise returns STATUS_OK
// usually there's no need to check the return value because an error code will be returned in any case
int errorobject_setwithfmt(struct Interpreter *interp, char *fmt, ...);

// never fails
#define errorobject_setnomem(interp) do { \
		OBJECT_INCREF((interp), (interp)->builtins.nomemerr); \
		(interp)->err = (interp)->builtins.nomemerr; \
	} while (0)

#endif    // OBJECTS_ERRORS_H
