// sets up interp->nomemerr, builtins and other stuff defined in various other files
#ifndef BUILTINS_H
#define BUILTINS_H

#include "interpreter.h"     // IWYU pragma: keep
#include "objectsystem.h"    // IWYU pragma: keep

/* returns STATUS_OK or STATUS_ERROR
interp->err doesn't exist when this is called, so that's always restored to NULL b4 this returns
if this returns STATUS_ERROR, the error message has been printed already
builtins_teardown() should be always called after this, regardless of the return value
*/
int builtins_setup(struct Interpreter *interp);

// never fails
void builtins_teardown(struct Interpreter *interp);

#endif    // BUILTINS_H
