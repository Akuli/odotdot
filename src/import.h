#ifndef IMPORT_H
#define IMPORT_H

#include <stdbool.h>

struct Interpreter;

// for interpreter_new() only, other files should use interp->stdlibpath
// returns a string that must be free()'d
// prints an error message to stderr and returns NULL on failure
// argv0 is used in error messages
char *import_findstdlib(char *argv0);

#endif    // IMPORT_H
