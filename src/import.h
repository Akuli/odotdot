#ifndef IMPORT_H
#define IMPORT_H

#include <stdbool.h>
#include "interpreter.h"   // IWYU pragma: keep

// for interpreter_new() only, other files should use interp->stdpath
// returns a string that must be free()'d
// prints an error message to stderr and returns NULL on failure
// argv0 is used in error messages
char *import_findstd(char *argv0);

// for builtins_setup()
// adds the built-in importer(s) to interp->importstuff.importers
// number of built-in importer(s) is an implementation detail
// throws an error and returns false on failure
bool import_init(struct Interpreter *interp);

#endif    // IMPORT_H
