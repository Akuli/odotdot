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
// TODO: definition context
struct Object *blockobject_new(struct Interpreter *interp, struct Object *definitionscope, struct Object *astnodearr);

#endif    // OBJECTS_BLOCK_H
