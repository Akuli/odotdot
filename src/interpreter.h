#ifndef INTERPRETER_H
#define INTERPRETER_H

// these are defined in other files that need to include this file
// stupid IWYU doesn't get this.....
struct Context;
struct Object;
struct ObjectClassInfo;

struct Interpreter {
	// this is set to argv[0] from main(), useful for error messages
	char *argv0;

	// some builtins are created in C, stdlib/fake_builtins.ö does others
	struct Context *builtinctx;

	// set errptr to this when there's not enough mem
	struct Object *nomemerr;

	// these are not exposed as built-in variables, so they can't be in builtinctx
	struct ObjectClassInfo *classobjectinfo;
	struct ObjectClassInfo *functionobjectinfo;
};

// returns NULL on no mem
// note that nomemerr is set to NULL!
struct Interpreter *interpreter_new(char *argv0);

// never fails
void interpreter_free(struct Interpreter *interp);

// convenience methods, these assert that name is valid UTF-8

// returns STATUS_OK or STATUS_ERROR
int interpreter_addbuiltin(struct Interpreter *interp, struct Object **errptr, char *name, struct Object *val);

// returns NULL and sets errptr on error
struct Object *interpreter_getbuiltin(struct Interpreter *interp, struct Object **errptr, char *name);

/* simple version of getbuiltin() that never calls malloc()
useful in e.g. builtins_teardown()
restrictions:
	* strlen(name) must be < 50
	* name must be ASCII only
	* no errptr, instead returns NULL if the variable doesn't exist
*/
struct Object *interpreter_getbuiltin_nomalloc(struct Interpreter *interp, char *name);

#endif   // INTERPRETER_H
