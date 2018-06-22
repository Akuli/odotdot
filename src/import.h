#ifndef IMPORT_H
#define IMPORT_H

#include <stdbool.h>

struct Interpreter;

// for interpreter_new() only, other files should use interp->stdlibpath
// returns a string that must be free()'d
// prints an error message to stderr and returns NULL on failure
// argv0 is used in error messages
char *import_findstdlib(char *argv0);

/* runs stdlib/builtins.ö
throws an error and returns false on failure

builtins.ö can't be ran like imported files because:
	* importing may set errors, but they're not available when running builtins.ö because it defines those errors
	* builtins.ö must be ran directly in the built-in scope
	* other files may contain any utf8 characters, but utf8_decode() also uses the errors

this is in the same place with import stuff because reading and running the file is similar
*/
bool import_runbuiltinsfile(struct Interpreter *interp);

// main.c calls this for running the program passed as an argument
// throws an error and returns false on failure
bool import_runmainfile(struct Interpreter *interp, char *path);

#endif    // IMPORT_H
