// this is not implemented in pure ö because stuff needs to be null before builtins.ö can run
// true and false are implemented in builtins.ö though :)
// TODO: would it make sense to have the same "marker object" type for true, false and null?

#ifndef OBJECTS_NULL_H
#define OBJECTS_NULL_H

struct Interpreter;
struct Object;

// RETURNS A NEW REFERENCE or NULL on error
struct Object *nullobject_create(struct Interpreter *interp);

// returns null AS A NEW REFERENCE, never fails
struct Object *nullobject_get(struct Interpreter *interp);

#endif    // OBJECTS_NULL_H
