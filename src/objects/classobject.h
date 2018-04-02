// a wrapper for ObjectClassInfo, that's why this file is named explicitly classobject.h
#ifndef OBJECTS_CLASSOBJECT_H
#define OBJECTS_CLASSOBJECT_H

#include "../interpreter.h"
#include "../objectsystem.h"

// sets interp->classobjectinfo and returns STATUS_OK or STATUS_ERROR
int classobject_createclass(struct Interpreter *interp, struct Object **errptr, struct ObjectClassInfo *objectclass);

// a wrapper for objectclassinfo_new()
struct Object *classobject_new(struct Interpreter *interp, struct Object **errptr, struct Object *baseclass, objectclassinfo_foreachref foreachref, void (*destructor)(struct Object *));

// a user-friendlier wrapper for object_new(), doesn't call ö setup() method
struct Object *classobject_newinstance(struct Interpreter *interp, struct Object **errptr, struct Object *klass, void *data);

// returns NULL on error
// the data is set to the ObjectClassInfo
struct Object *classobject_newfromclassinfo(struct Interpreter *interp, struct Object **errptr, struct ObjectClassInfo *wrapped);

#endif    // OBJECTS_CLASSOBJECT_H
