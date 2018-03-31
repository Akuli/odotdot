#ifndef INTERPRETER_H
#define INTERPRETER_H

#include "objectsystem.h"   // IWYU pragma: keep

struct Context;     // defined in context.h
struct Interpreter {
	// this is set to argv[0] from main(), useful for error messages
	char *argv0;

	// some builtins are created in C, stdlib/fake_builtins.ö does others
	struct Context *builtinctx;

	// set errptr to this when there's not enough mem
	struct Object *nomemerr;
};

// returns NULL on no mem
// note that nomemerr is set to NULL!
struct Interpreter *interpreter_new(char *argv0);

// never fails
void interpreter_free(struct Interpreter *interp);

// convenience methods, these assert that name is valid UTF-8

// returns STATUS_OK or STATUS_ERROR
int interpreter_addbuiltin(struct Interpreter *interp, struct Object **errptr, char *name, struct Object *val);

// returns NULL on error
struct Object *interpreter_getbuiltin(struct Interpreter *interp, struct Object **errptr, char *name);

#endif   // INTERPRETER_H
