#ifndef OBJECTS_INTEGER_H
#define OBJECTS_INTEGER_H

#include "../interpreter.h"    // IWYU pragma: keep
#include "../objectsystem.h"   // IWYU pragma: keep
#include "../unicode.h"        // IWYU pragma: keep


// long long is at least 64 bits
// ö integers are never more than 64 bits for consistency on different platforms
// INT64_MIN and INT64_MAX macros don't exist on systems that don't have int64_t
#define INTEGEROBJECT_MAX 0x7fffffffffffffffLL

// careful here... INTEGEROBJECT_MAX+1 is undefined, but (-INTEGEROBJECT_MAX)-1 is ok
#define INTEGEROBJECT_MIN ((-INTEGEROBJECT_MAX) - 1)


// decimal representations of all 64-bit ints without leading zeros fit in 20 characters
#define INTEGER_MAXLEN 20


// RETURNS A NEW REFERENCE or NULL on error
struct Object *integerobject_createclass(struct Interpreter *interp, struct Object **errptr);

// doesn't change ustr
// RETURNS A NEW REFERENCE
struct Object *integerobject_newfromustr(struct Interpreter *interp, struct Object **errptr, struct UnicodeString ustr);

// asserts that the char pointer is digits only, i.e. don't use with user-inputted strings
// RETURNS A NEW REFERENCE
struct Object *integerobject_newfromcharptr(struct Interpreter *interp, struct Object **errptr, char *s);

// if the object is an integer, never fails
// if the object is not an integer, bad things happen
long long integerobject_tolonglong(struct Object *integer);


#endif   // OBJECTS_INTEGER_H
