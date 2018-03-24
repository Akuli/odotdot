#ifndef OBJECTS_FUNCTION_H
#define OBJECTS_FUNCTION_H

#include "../objectsystem.h"


// should return STATUS_OK or STATUS_NOMEM
typedef int (*functionobject_cfunc)(struct Object **retval);

struct ObjectClassInfo *functionobject_createclass(void);
struct Object *functionobject_new(functionobject_cfunc func);


#endif   // OBJECTS_FUNCTION_H
