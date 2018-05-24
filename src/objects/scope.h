#ifndef OBJECTS_SCOPE_H
#define OBJECTS_SCOPE_H

#include "../interpreter.h"     // IWYU pragma: keep
#include "../objectsystem.h"    // IWYU pragma: keep

// RETURNS A NEW REFERENCE or NULL on error
struct Object *scopeobject_createclass(struct Interpreter *interp);

// RETURNS A NEW REFERENCE
struct Object *scopeobject_newbuiltin(struct Interpreter *interp);

// bad things happen if parentscope is not a scope object
// RETURNS A NEW REFERENCE
struct Object *scopeobject_newsub(struct Interpreter *interp, struct Object *parentscope);

#endif   // OBJECTS_SCOPE_H
