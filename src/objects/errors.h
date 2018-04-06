// TODO: add subclasses of Error
#ifndef OBJECTS_ERRORS_H
#define OBJECTS_ERRORS_H

#include "../objectsystem.h"    // IWYU pragma: keep

// returns NULL on no mem
// doesn't take errptr because errors don't exist yet when this is called
struct ObjectClassInfo *errorobject_createclass(struct ObjectClassInfo *objectclass);

// returns NULL on no mem
// RETURNS A NEW REFERENCE
struct Object *errorobject_createnomemerr(struct Interpreter *interp, struct ObjectClassInfo *errorclass, struct ObjectClassInfo *stringclass);

#endif    // OBJECTS_ERRORS_H
