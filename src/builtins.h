// sets up interp->nomemerr, builtins and other stuff defined in various other files
#ifndef BUILTINS_H
#define BUILTINS_H

#include "interpreter.h"     // IWYU pragma: keep
#include "objectsystem.h"    // IWYU pragma: keep

// returns STATUS_OK, STATUS_ERROR or STATUS_NOMEM because interp->errptr
// doesn't need to exist when this is called
int builtins_setup(struct Interpreter *interp, struct Object **errptr);

// never fails
void builtins_teardown(struct Interpreter *interp);

#endif    // BUILTINS_H
