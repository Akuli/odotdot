// method-related utility functions
#ifndef METHOD_H
#define METHOD_H

#include "interpreter.h"       // IWYU pragma: keep
#include "objectsystem.h"      // IWYU pragma: keep
#include "objects/function.h"  // IWYU pragma: keep
#include "unicode.h"           // IWYU pragma: keep
#include <stdbool.h>

// returns false on error
// name must be valid UTF-8
// bad things happen if klass is not a class object
// an assert fails if name is long
// you need to use check_args to make sure that bad things don't happen if someone calls the functions weirdly
// cfunc is called with the ObjectData's .data set to the 'this' object, so you can do e.g.:
//
//    struct Object *some_method(struct Interpreter *interp, struct ObjectData thisdata, struct Object *args, struct Object *opts)
//    {
//        struct Object *this = thisdata.data;
//        ....
bool method_add_yesret(struct Interpreter *interp, struct Object *klass, char *name, functionobject_cfunc_yesret cfunc);
bool method_add_noret(struct Interpreter *interp, struct Object *klass, char *name, functionobject_cfunc_noret cfunc);

// RETURNS A NEW REFERENCE or NULL on error
// name must be valid utf-8
struct Object *method_get(struct Interpreter *interp, struct Object *obj, char *name);

// RETURNS A NEW REFERENCE or NULL on error
struct Object *method_getwithustr(struct Interpreter *interp, struct Object *obj, struct UnicodeString name);

// convenience functions for calling methods
// see also functionobject_call
// there is no method_vcall, use method_get with functionobject_vcall
struct Object *method_call_yesret(struct Interpreter *interp, struct Object *obj, char *methname, ...);
bool method_call_noret(struct Interpreter *interp, struct Object *obj, char *methname, ...);

// these call "special" methods and check the return type
// these RETURN A NEW REFERENCE or NULL on error
struct Object *method_call_tostring(struct Interpreter *interp, struct Object *obj);
struct Object *method_call_todebugstring(struct Interpreter *interp, struct Object *obj);

#endif    // METHOD_H
