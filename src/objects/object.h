#ifndef OBJECTS_OBJECT_H
#define OBJECTS_OBJECT_H

#include "../objectsystem.h"    // IWYU pragma: keep

// this is not a copy/pasta, this creates the Object baseclass-of-everything
// other wat_createclass() functions are named e.g. functionobject_createclass()
struct ObjectClassInfo *objectobject_createclass(void);

#endif   // OBJECTS_OBJECT_H
