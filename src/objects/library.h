#ifndef OBJECTS_LIBRARY_H
#define OBJECTS_LIBRARY_H

#include "../interpreter.h"    // IWYU pragma: keep
#include "../objectsystem.h"   // IWYU pragma: keep

// creates the ArbitraryAttribs class
struct Object *libraryobject_createaaclass(struct Interpreter *interp);

// creates the Library class using interp->builtins.ArbitraryAttribs
struct Object *libraryobject_createclass(struct Interpreter *interp);

#endif    // OBJECTS_LIBRARY_H
