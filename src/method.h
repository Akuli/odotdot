// method-related utility functions
#ifndef METHOD_H
#define METHOD_H

#include "interpreter.h"       // IWYU pragma: keep
#include "objectsystem.h"      // IWYU pragma: keep
#include "objects/function.h"
#include "unicode.h"

// returns false on error
// name must be valid UTF-8
// bad things happen if klass is not a class object or name is very long
bool method_add(struct Interpreter *interp, struct Object *klass, char *name, functionobject_cfunc cfunc);

// RETURNS A NEW REFERENCE or NULL on error
// name must be valid utf-8
struct Object *method_get(struct Interpreter *interp, struct Object *obj, char *name);

// RETURNS A NEW REFERENCE or NULL on error
struct Object *method_getwithustr(struct Interpreter *interp, struct Object *obj, struct UnicodeString name);

// the ultimate convenience method for calling methods
// see also functionobject_call
// there is no method_vcall, use method_get with functionobject_vcall
// RETURNS A NEW REFERENCE or NULL on error
struct Object *method_call(struct Interpreter *interp, struct Object *obj, char *methname, ...);

// these call "special" methods and check the return type
// these RETURN A NEW REFERENCE or NULL on error
struct Object *method_call_tostring(struct Interpreter *interp, struct Object *obj);
struct Object *method_call_todebugstring(struct Interpreter *interp, struct Object *obj);

#endif    // METHOD_H
