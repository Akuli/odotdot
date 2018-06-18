#ifndef OBJECTS_INTEGER_H
#define OBJECTS_INTEGER_H

#include "../interpreter.h"    // IWYU pragma: keep
#include "../objectsystem.h"   // IWYU pragma: keep
#include "../unicode.h"        // IWYU pragma: keep


// long long is at least 64 bits
// but the standard doesn't say anything about one's complement vs two's complement
// so maximum is 2^63 - 1 and minimum is -(2^63 - 1)
#define INTEGEROBJECT_MIN (-0x7fffffffffffffffLL)
#define INTEGEROBJECT_MAX   0x7fffffffffffffffLL
#define INTEGEROBJECT_MINSTR "-9223372036854775807"
#define INTEGEROBJECT_MAXSTR  "9223372036854775807"

// INTEGEROBJECT_MINSTR, INTEGEROBJECT_MAXSTR and everythign between them fit in this many chars
#define INTEGEROBJECT_MAXDIGITS 20


// RETURNS A NEW REFERENCE or NULL on error
struct Object *integerobject_createclass(struct Interpreter *interp);

// asserts that the value fits in a 64-bit int, long long can be more than 64 bits in c99
// RETURNS A NEW REFERENCE or NULL on error
struct Object *integerobject_newfromlonglong(struct Interpreter *interp, long long val);

// RETURNS A NEW REFERENCE
struct Object *integerobject_newfromustr(struct Interpreter *interp, struct UnicodeString ustr);

// if the object is an integer, never fails
// if the object is not an integer, bad things happen
long long integerobject_tolonglong(struct Object *integer);


#endif   // OBJECTS_INTEGER_H
