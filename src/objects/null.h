// this is not implemented in pure ö because stuff needs to be null before builtins.ö can run
// TODO: would it make sense to have the same "marker object" type for true, false and null?

#ifndef OBJECTS_NULL_H
#define OBJECTS_NULL_H

#include "../interpreter.h"   // IWYU pragma: keep
#include "../objectsystem.h"  // IWYU pragma: keep

// RETURNS A NEW REFERENCE or NULL on no mem
struct Object *nullobject_create_noerr(struct Interpreter *interp);

// returns null AS A NEW REFERENCE, never fails
struct Object *nullobject_get(struct Interpreter *interp);

#endif    // OBJECTS_NULL_H
