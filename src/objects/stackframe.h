#ifndef OBJECTS_STACKFRAME_H
#define OBJECTS_STACKFRAME_H

// these could be fwd dcls, except that it's possible to IWYU ignore includes but not fwd dcls
#include "../interpreter.h"    // IWYU pragma: keep
#include "../objectsystem.h"   // IWYU pragma: keep

// RETURNS A NEW REFERENCE or NULL on error
struct Object *stackframeobject_createclass(struct Interpreter *interp);

// converts interp->stack to an array of StackFrame objects
// RETURNS A NEW REFERENCE or NULL on error
struct Object *stackframeobject_getstack(struct Interpreter *interp);


#endif   // OBJECTS_STACKFRAME_H

