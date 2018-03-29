#ifndef OBJECTS_INTEGER_H
#define OBJECTS_INTEGER_H

// iwyu doesn't understand these includes
#include "../objectsystem.h"    // for Object and ObjectClassInfo
#include "../unicode.h"    // for UnicodeString

struct ObjectClassInfo *integerobject_createclass(struct ObjectClassInfo *objectclass);

// doesn't change ustr
struct Object *integerobject_newfromustr(struct ObjectClassInfo *integerclass, struct UnicodeString ustr);

// asserts that the char pointer is digits only, i.e. don't use with user-inputted strings
struct Object *integerobject_newfromcharptr(struct ObjectClassInfo *integerclass, char *ptr);

// never fails
int64_t integerobject_toint64(struct Object *integer);


#endif   // OBJECTS_INTEGER_H
