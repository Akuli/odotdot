#ifndef RUNAST_H
#define RUNAST_H

#include <stdbool.h>

#include "interpreter.h"     // IWYU pragma: keep
#include "objectsystem.h"     // IWYU pragma: keep

// runs a statement from ast_parse_statement()
// returns false on error
// bad things happen if scope is not a Scope object or stmtnode is not an AstNode
bool runast_statement(struct Interpreter *interp, struct Object *scope, struct Object *stmtnode);

#endif    // RUNAST_H
