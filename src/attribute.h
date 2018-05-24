#ifndef ATTRIBUTE_H
#define ATTRIBUTE_H

#include "interpreter.h"    // IWYU pragma: keep
#include "objectsystem.h"   // IWYU pragma: keep
#include "unicode.h"        // IWYU pragma: keep

// RETURNS A NEW REFERENCE or NULL on error
struct Object *attribute_get(struct Interpreter *interp, struct Object *obj, char *attr);

// RETURNS A NEW REFERENCE or NULL on error
struct Object *attribute_getwithustr(struct Interpreter *interp, struct Object *obj, struct UnicodeString attr);

// returns STATUS_OK or STATUS_ERROR
int attribute_set(struct Interpreter *interp, struct Object *obj, char *attr, struct Object *val);

#endif     // ATTRIBUTE_H
