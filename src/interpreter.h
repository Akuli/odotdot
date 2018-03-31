#ifndef INTERPRETER_H
#define INTERPRETER_H

#include "objectsystem.h"   // IWYU pragma: keep

struct Interpreter {
	// keys are UnicodeString pointers, values can be any objects
	struct HashTable *globalvars;

	// set errptr to this when there's not enough mem
	struct Object *nomemerr;
};

// returns NULL on no mem
// note that nomemerr is set to NULL!
struct Interpreter *interpreter_new(void);

// never fails
void interpreter_free(struct Interpreter *interp);

// convenience methods, the name must be valid utf8

// returns STATUS_OK or STATUS_ERROR
int interpreter_addglobal(struct Interpreter *interp, struct Object **errptr, char *name, struct Object *val);

// returns NULL and sets *errptr on no mem
struct Object *interpreter_getglobal(struct Interpreter *interp, struct Object **errptr, char *name);

#endif   // INTERPRETER_H
