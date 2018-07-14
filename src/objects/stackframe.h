#ifndef OBJECTS_STACKFRAME_H
#define OBJECTS_STACKFRAME_H

// these could be fwd dcls, except that it's possible to IWYU ignore includes but not fwd dcls
#include "../interpreter.h"    // IWYU pragma: keep
#include "../objectsystem.h"   // IWYU pragma: keep

// RETURNS A NEW REFERENCE or NULL on error
struct Object *stackframeobject_createclass(struct Interpreter *interp);

// adds interp->stack to an array as StackFrame objects
bool stackframeobject_getstack(struct Interpreter *interp, struct Object *arr);


#endif   // OBJECTS_STACKFRAME_H

