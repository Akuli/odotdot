// this is not implemented in pure ö because stuff needs to be null before builtins.ö can run
// TODO: would it make sense to have the same "marker object" type for true, false and null?

#ifndef OBJECTS_NULL_H
#define OBJECTS_NULL_H

#include <stdbool.h>

struct Interpreter;
struct Object;

// RETURNS A NEW REFERENCE or NULL on no mem
struct Object *nullobject_create_noerr(struct Interpreter *interp);

// returns false and throws an error on failure
bool nullobject_addmethods(struct Interpreter *interp);

// returns null AS A NEW REFERENCE, never fails
struct Object *nullobject_get(struct Interpreter *interp);

#endif    // OBJECTS_NULL_H
