#ifndef OBJECTS_ARRAY_H
#define OBJECTS_ARRAY_H

#include <stddef.h>
#include "../objectsystem.h"    // IWYU pragma: keep

// these return NULL on no mem
struct ObjectClassInfo *arrayobject_createclass(struct ObjectClassInfo *objectclass);
struct Object *arrayobject_newempty(struct ObjectClassInfo *arrayclass);

// these return NULL on no mem and always make a copy of the given array
struct Object *arrayobject_newfromarrayandsize(struct ObjectClassInfo *arrayclass, struct Object **array, size_t size);
struct Object *arrayobject_newfromnullterminated(struct ObjectClassInfo *arrayclass, struct Object **nullterminated);

#endif    // OBJECTS_ARRAY_H
