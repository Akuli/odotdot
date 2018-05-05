// sets up interp->nomemerr, builtins and other stuff defined in various other files
#ifndef BUILTINS_H
#define BUILTINS_H

#include "interpreter.h"     // IWYU pragma: keep
#include "objectsystem.h"    // IWYU pragma: keep

/* returns STATUS_OK, STATUS_ERROR or STATUS_NOMEM
interp->errptr doesn't exist when this is called
if this returns STATUS_NOMEM, an error message should be printed
if this returns STATUS_ERROR, the error message has been printed already
builtins_teardown() should be always called after this, regardless of the return value
*/
int builtins_setup(struct Interpreter *interp);

// never fails
void builtins_teardown(struct Interpreter *interp);

#endif    // BUILTINS_H
