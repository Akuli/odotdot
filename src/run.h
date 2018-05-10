#ifndef RUN_H
#define RUN_H

#include "ast.h"              // IWYU pragma: keep
#include "context.h"          // IWYU pragma: keep
#include "objectsystem.h"     // IWYU pragma: keep

// runs a statement from ast_parse_statement(), returns STATUS_OK or STATUS_ERROR
int run_statement(struct Context *ctx, struct Object **errptr, struct Object *stmtnode);

#endif    // RUN_H
