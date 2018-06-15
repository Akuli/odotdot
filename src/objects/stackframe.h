#ifndef OBJECTS_STACKFRAME_H
#define OBJECTS_STACKFRAME_H

struct Interpreter;
struct Object;


// RETURNS A NEW REFERENCE or NULL on error
struct Object *stackframeobject_createclass(struct Interpreter *interp);

// converts interp->stack to an array of StackFrame objects
// RETURNS A NEW REFERENCE or NULL on error
struct Object *stackframeobject_getstack(struct Interpreter *interp);


#endif   // OBJECTS_STACKFRAME_H

