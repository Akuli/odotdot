#ifndef OBJECTS_INTEGER_H
#define OBJECTS_INTEGER_H

#include <stdint.h>
#include "../interpreter.h"    // IWYU pragma: keep
#include "../objectsystem.h"   // IWYU pragma: keep
#include "../unicode.h"        // IWYU pragma: keep

struct ObjectClassInfo *integerobject_createclass(struct ObjectClassInfo *objectclass);

// doesn't change ustr
struct Object *integerobject_newfromustr(struct Interpreter *interp, struct ObjectClassInfo *integerclass, struct UnicodeString ustr);

// asserts that the char pointer is digits only, i.e. don't use with user-inputted strings
struct Object *integerobject_newfromcharptr(struct Interpreter *interp, struct ObjectClassInfo *integerclass, char *ptr);

// never fails
int64_t integerobject_toint64(struct Object *integer);


#endif   // OBJECTS_INTEGER_H
