// method-related utility functions
#ifndef METHOD_H
#define METHOD_H

#include "interpreter.h"
#include "objectsystem.h"
#include "objects/function.h"

// returns STATUS_OK or STATUS_ERROR
// name must be valid UTF-8
int method_add(struct Interpreter *interp, struct Object **errptr, struct Object *klass, char *name, functionobject_cfunc cfunc);

// RETURNS A NEW REFERENCE or NULL on error
// name must be valid utf-8
struct Object *method_get(struct Interpreter *interp, struct Object **errptr, struct Object *obj, char *name);

// RETURNS A NEW REFERENCE or NULL on error
struct Object *method_getwithustr(struct Interpreter *interp, struct Object **errptr, struct Object *obj, struct UnicodeString name);

#endif    // METHOD_H
