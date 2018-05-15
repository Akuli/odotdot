// stupid garbage collector

#ifndef GC_H
#define GC_H

#include "interpreter.h"     // IWYU pragma: keep

// removes reference cycles when the interpreter exists
// can't be invoked at runtime, never fails
void gc_run(struct Interpreter *interp);

#endif   // GC_H
