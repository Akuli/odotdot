#ifndef INTERPRETER_H
#define INTERPRETER_H

#include "common.h"
#include "hashtable.h"
#include "objectsystem.h"

struct Interpreter {
	// keys are String objects, values can be any objects
	struct HashTable *globalvars;
};

// returns NULL on no mem
struct Interpreter *interpreter_new(void);

// never fails
void interpreter_free(struct Interpreter *interp);

/*// a convenience method, returns STATUS_OK or STATUS_NOMEM
// assumes that the String global has been added
int interpreter_addglobal(struct Interpreter *interp, char *name, int *val);

// returns STATUS_OK or STATUS_NOMEM, res is set to NULL if not found
int interpreter_getglobal(char *name, struct Object **res);
*/

#endif   // INTERPRETER_H
