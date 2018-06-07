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
// bad things happen if definition_scope is not a Scope or astnodearr is not an Array of AstNodes
struct Object *blockobject_new(struct Interpreter *interp, struct Object *definition_scope, struct Object *astnodearr);

// returns false on error
// bad things happen if block is not a Block object or scope is not a Scope object
bool blockobject_run(struct Interpreter *interp, struct Object *block, struct Object *scope);

#endif    // OBJECTS_BLOCK_H
