#ifndef ATTRIBUTE_H
#define ATTRIBUTE_H

#include "interpreter.h"    // IWYU pragma: keep
#include "objectsystem.h"   // IWYU pragma: keep
#include "unicode.h"        // IWYU pragma: keep
#include "objects/function.h"

// returns STATUS_OK or STATUS_ERROR
// setter and getter can be NULL (but not both, that would do nothing)
// bad things happen if klass is not a class object
int attribute_add(struct Interpreter *interp, struct Object *klass, char *name, functionobject_cfunc getter, functionobject_cfunc setter);

// these RETURN A NEW REFERENCE or NULL on error
struct Object *attribute_get(struct Interpreter *interp, struct Object *obj, char *attr);
struct Object *attribute_getwithstringobj(struct Interpreter *interp, struct Object *obj, struct Object *stringobj);

// these return STATUS_OK or STATUS_ERROR
int attribute_set(struct Interpreter *interp, struct Object *obj, char *attr, struct Object *val);
int attribute_setwithstringobj(struct Interpreter *interp, struct Object *obj, struct Object *stringobj, struct Object *val);

// returns STATUS_OK or STATUS_ERROR
int attribute_set(struct Interpreter *interp, struct Object *obj, char *attr, struct Object *val);

#endif     // ATTRIBUTE_H
