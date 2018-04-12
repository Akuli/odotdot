// TODO: add subclasses of Error
#ifndef OBJECTS_ERRORS_H
#define OBJECTS_ERRORS_H

#include "../context.h"         // IWYU pragma: keep
#include "../interpreter.h"     // IWYU pragma: keep
#include "../objectsystem.h"    // IWYU pragma: keep

// returns NULL on no mem
// doesn't take errptr because errors don't exist yet when this is called
struct ObjectClassInfo *errorobject_createclass(struct ObjectClassInfo *objectclass);

// RETURNS A NEW REFERENCE or NULL on no mem
struct Object *errorobject_createnomemerr(struct Interpreter *interp, struct ObjectClassInfo *errorclass, struct ObjectClassInfo *stringclass);

// the ultimate convenience function
// sets errptr to an error with a string created with stringobject_newfromfmt
// it was named errorobject_seterrptrfromfmtstring but that was way too long to type
// i thought about errorobject_sethandy, but that wouldn't be very descriptive and setfmt is even shorter
// on failure, sets errptr to some other error and returns STATUS_ERROR, otherwise returns STATUS_OK
// usually there's no need to check the return value because an error code will be returned in any case
int errorobject_setwithfmt(struct Context *ctx, struct Object **errptr, char *fmt, ...);

/* does this:

	OBJECT_INCREF(interp, interp->nomemerr);
	*errptr = interp->nomemerr;

never fails
*/
void errorobject_setnomem(struct Interpreter *interp, struct Object **errptr);

// sets an error and returns STATUS_ERROR if obj is not an instance of klass
// returns STATUS_OK on success
int errorobject_typecheck(struct Context *ctx, struct Object **errptr, struct Object *klass, struct Object *obj);

#endif    // OBJECTS_ERRORS_H
