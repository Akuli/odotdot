// a wrapper for ObjectClassInfo, that's why this file is named explicitly classobject.h
#ifndef OBJECTS_CLASSOBJECT_H
#define OBJECTS_CLASSOBJECT_H

#include "../objectsystem.h"

struct ObjectClassInfo *classobject_createclass(struct ObjectClassInfo *objectclass);

// returns NULL on no mem
// the data is set to the ObjectClassInfo
struct Object *classobject_newfromclassinfo(struct ObjectClassInfo *classobjectclass, struct ObjectClassInfo *wrapped);

#endif    // OBJECTS_CLASSOBJECT_H
