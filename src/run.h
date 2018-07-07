#ifndef RUN_H
#define RUN_H

#include <stdbool.h>
#include "interpreter.h"     // IWYU pragma: keep
#include "objectsystem.h"    // IWYU pragma: keep

/* runs std/builtins.ö
throws an error and returns false on failure

builtins.ö can't be ran like imported files because:
	* importing may set errors, but they're not available when running builtins.ö because it defines those errors
	* builtins.ö must be ran directly in the built-in scope
	* other files may contain any utf8 characters, but utf8_decode() also uses the errors

this is in the same place with import stuff because reading and running the file is similar
*/
bool run_builtinsfile(struct Interpreter *interp);

// main.c calls this for running the program passed as an argument
// the path does not need to be absolute
// throws an error and returns false on failure
bool run_mainfile(struct Interpreter *interp, char *path);

// for import.c, abspath must be absolute
// runs a file in a scope that contains an export function
// exported variables are added as attributes to lib
//
// return values:
//  1   success
//  0   an error has been thrown
//  -1  opening the file returned ENOENT, the file doesn't exist
int run_libfile(struct Interpreter *interp, char *abspath, struct Object *lib);

#endif   // RUN_H
