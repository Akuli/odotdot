#ifndef RUN_H
#define RUN_H

#include "interpreter.h"     // IWYU pragma: keep
#include "objectsystem.h"     // IWYU pragma: keep

// runs a statement from ast_parse_statement(), returns STATUS_OK or STATUS_ERROR
// bad things happen if scope is not a Scope object or stmtnode is not an AstNode
int run_statement(struct Interpreter *interp, struct Object **errptr, struct Object *scope, struct Object *stmtnode);

#endif    // RUN_H
