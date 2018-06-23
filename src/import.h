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

// for builtins_setup()
// adds the built-in importer(s) to interp->importstuff.importers
// number of built-in importer(s) is an implementation detail
// throws an error and returns false on failure
bool import_init(struct Interpreter *interp);

// returns a Module object, sets an error and returns NULL on failure
// stackframe should be the StackFrame object that import was called from, used for e.g. finding files relatively
struct Object *import(struct Interpreter *interp, struct Object *namestring, struct Object *stackframe);

#endif    // IMPORT_H
