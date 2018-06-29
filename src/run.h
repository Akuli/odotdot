#ifndef RUN_H
#define RUN_H

#include <stdbool.h>

struct Interpreter;
struct Object;

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

// for import.c, the path must be absolute
// sets a Mapping of variables defined in the file to vars on success
// return values:
//  1   success
//  0   an error has been thrown
//  -1  opening the file returned ENOENT, the file doesn't exist
// throws an error and returns NULL on failure
int run_libfile(struct Interpreter *interp, char *abspath, struct Object *scope);

#endif   // RUN_H
