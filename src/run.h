#ifndef RUN_H
#define RUN_H

#include "ast.h"              // IWYU pragma: keep
#include "context.h"          // IWYU pragma: keep
#include "objectsystem.h"     // IWYU pragma: keep

// runs a statement, returns STATUS_OK or STATUS_ERROR
int run_statement(struct Context *ctx, struct Object **errptr, struct AstNode *stmt);

#endif    // RUN_H
