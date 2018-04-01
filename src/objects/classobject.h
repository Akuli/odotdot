// a wrapper for ObjectClassInfo, that's why this file is named explicitly classobject.h
#ifndef OBJECTS_CLASSOBJECT_H
#define OBJECTS_CLASSOBJECT_H

#include "../interpreter.h"
#include "../objectsystem.h"

// returns NULL on no mem
struct ObjectClassInfo *classobject_createclass(struct Interpreter *interp, struct Object **errptr, struct ObjectClassInfo *objectclass);

// a wrapper for objectclassinfo_new()
struct Object *classobject_new(struct Interpreter *interp, struct Object **errptr, struct Object *baseclass, objectclassinfo_foreachref foreachref, void (*destructor)(struct Object *));

// returns NULL on error
// the data is set to the ObjectClassInfo
struct Object *classobject_newfromclassinfo(struct Interpreter *interp, struct Object **errptr, struct ObjectClassInfo *wrapped);

#endif    // OBJECTS_CLASSOBJECT_H
