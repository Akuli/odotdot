#ifndef OBJECTS_BLOCK_H
#define OBJECTS_BLOCK_H

#include <stddef.h>
#include "../interpreter.h"    // IWYU pragma: keep
#include "../objectsystem.h"   // IWYU pragma: keep

// the data of block objects is NULL, instead they have an ast_nodes attribute set to an array object
// TODO: add a definition_context attribute?

// RETURNS A NEW REFERENCE or NULL on error
struct Object *blockobject_createclass(struct Interpreter *interp);

// RETURNS A NEW REFERENCE or NULL on error
struct Object *blockobject_new(struct Interpreter *interp, struct Object *definitionscope, struct Object *astnodearr);

// returns STATUS_OK or STATUS_ERROR
// bad things happen if block is not a Block object or scope is not a Scope object
int blockobject_run(struct Interpreter *interp, struct Object *block, struct Object *scope);

#endif    // OBJECTS_BLOCK_H
