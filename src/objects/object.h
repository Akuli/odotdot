/* repeated object is not a copy/pasta
this file represents the Object baseclass-of-everything
other files have things like OBJECTS_STRING_H and stringobject_blahblah() */

#ifndef OBJECTS_OBJECT_H
#define OBJECTS_OBJECT_H

#include "../interpreter.h"     // IWYU pragma: keep
#include "../objectsystem.h"    // IWYU pragma: keep

// only called from builtins_setup()
// RETURNS A NEW REFERENCE or NULL on no mem
struct Object *objectobject_createclass_noerr(struct Interpreter *interp);

// returns STATUS_OK or STATUS_ERROR
// see builtins_setup()
int objectobject_addmethods(struct Interpreter *interp);

#endif   // OBJECTS_OBJECT_H
