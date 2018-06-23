#ifndef IMPORT_H
#define IMPORT_H

#include <stdbool.h>
#include "unicode.h"

struct Interpreter;
struct Object;

// for interpreter_new() only, other files should use interp->stdlibspath
// returns a string that must be free()'d
// prints an error message to stderr and returns NULL on failure
// argv0 is used in error messages
char *import_findstdlibs(char *argv0);

// returns a Module object, sets an error and returns NULL on failure
// sourcedir should be the directory where the file that import was called from is
struct Object *import(struct Interpreter *interp, struct UnicodeString name, char *sourcedir);

#endif    // IMPORT_H
