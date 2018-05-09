#ifndef OBJECTS_ARRAY_H
#define OBJECTS_ARRAY_H

#include <stddef.h>
#include "../interpreter.h"    // IWYU pragma: keep
#include "../objectsystem.h"    // IWYU pragma: keep

// RETURNS A NEW REFERENCE or NULL on error
struct Object *arrayobject_createclass(struct Interpreter *interp, struct Object **errptr);

// RETURNS A NEW REFERENCE or NULL on error
struct Object *arrayobject_new(struct Interpreter *interp, struct Object **errptr, struct Object **elems, size_t nelems);

// creates NEW REFERENCES to each object
// RETURNS A NEW REFERENCE or NULL on error
struct Object *arrayobject_newempty(struct Interpreter *interp, struct Object **errptr);

#endif    // OBJECTS_ARRAY_H
