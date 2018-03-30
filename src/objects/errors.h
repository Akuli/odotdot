// TODO: add subclasses of Error
#ifndef OBJECTS_ERRORS_H
#define OBJECTS_ERRORS_H

#include "../objectsystem.h"

// returns NULL on no mem
struct ObjectClassInfo *errorobject_createclass(struct ObjectClassInfo *objectclass);

// returns NULL on no mem
struct Object *errorobject_newfromstringobject(struct ObjectClassInfo *errorclass, struct Object *msgstring);

// a convenience method, assumes that the char pointer is valid UTF-8
// returns NULL on no mem
struct Object *errorobject_newfromcharptr(struct ObjectClassInfo *errorclass, struct ObjectClassInfo *stringclass, char *msg);

#endif    // OBJECTS_ERRORS_H
