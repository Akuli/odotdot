#ifndef OBJECTS_INTEGER_H
#define OBJECTS_INTEGER_H

#include <stdint.h>
#include "../interpreter.h"    // IWYU pragma: keep
#include "../objectsystem.h"   // IWYU pragma: keep
#include "../unicode.h"        // IWYU pragma: keep


// decimal representations of all 64-bit ints without leading zeros fit in 20 characters
#define INTEGER_MAXLEN 20


// returns STATUS_OK or STATUS_ERROR
int integerobject_createclass(struct Interpreter *interp, struct Object **errptr);

// doesn't change ustr
// RETURNS A NEW REFERENCE
struct Object *integerobject_newfromustr(struct Interpreter *interp, struct Object **errptr, struct UnicodeString ustr);

// asserts that the char pointer is digits only, i.e. don't use with user-inputted strings
// RETURNS A NEW REFERENCE
struct Object *integerobject_newfromcharptr(struct Interpreter *interp, struct Object **errptr, char *s);

// assumes that the object is an integer, never fails
int64_t integerobject_toint64(struct Object *integer);


#endif   // OBJECTS_INTEGER_H
