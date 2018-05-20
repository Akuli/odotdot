#ifndef ARRAYFUNC_H
#define ARRAYFUNC_H

#include "interpreter.h"     // IWYU pragma: keep
#include "objectsystem.h"    // IWYU pragma: keep

// create the array_func builtin
// RETURNS A NEW REFERENCE or NULL on error
struct Object *arrayfunc_create(struct Interpreter *interp, struct Object **errptr);

#endif    // ARRAYFUNC_H
