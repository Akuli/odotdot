// this is not implemented in pure ö because it's needed before builtins.ö can run
#ifndef OBJECTS_OPTION_H
#define OBJECTS_OPTION_H

#include "../interpreter.h"   // IWYU pragma: keep
#include "../objectsystem.h"  // IWYU pragma: keep

// these are for builtins_setup()
struct Object *optionobject_createclass_noerr(struct Interpreter *interp);
struct Object *optionobject_createnone_noerr(struct Interpreter *interp);

// see builtins_setup()
// throws an error and returns false on failure
bool optionobject_addmethods(struct Interpreter *interp);

#endif    // OBJECTS_OPTION_H
