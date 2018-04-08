/* repeated object is not a copy/pasta
this file represents the object baseclass-of-everything
other files have things like OBJECTS_STRING_H */

#ifndef OBJECTS_OBJECT_H
#define OBJECTS_OBJECT_H

#include "../objectsystem.h"    // IWYU pragma: keep

// returns NULL for no mem because interp->nomemerr is NULL when this is called
struct ObjectClassInfo *objectobject_createclass(void);

// returns STATUS_OK or STATUS_NOMEM
// this uses things that don't exist when objectobject_createclass() is called
// see builtins_setup()
int objectobject_addmethods(struct Interpreter *interp, struct Object **errptr);

#endif   // OBJECTS_OBJECT_H
