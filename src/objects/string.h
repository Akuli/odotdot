#ifndef OBJECTS_STRING_H
#define OBJECTS_STRING_H

// iwyu doesn't understand these includes
#include "../objectsystem.h"    // for Object and ObjectClassInfo
#include "../unicode.h"    // for UnicodeString

struct ObjectClassInfo *stringobject_createclass(struct ObjectClassInfo *objectclass);

// makes a copy of ustr with unicodestring_copy()
struct Object *stringobject_newfromustr(struct ObjectClassInfo *stringclass, struct UnicodeString ustr);

// asserts that the char pointer is valid utf8, i.e. don't use with user-inputted strings
struct Object *stringobject_newfromcharptr(struct ObjectClassInfo *stringclass, char *ptr);

// the returned UnicodeString's val must NOT be free()'d
struct UnicodeString *stringobject_getustr(struct Object *str);


#endif   // OBJECTS_STRING_H
