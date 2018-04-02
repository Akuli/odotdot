#ifndef RUN_H
#define RUN_H

#include "ast.h"
#include "context.h"
#include "objectsystem.h"

// runs a statement, returns STATUS_OK or STATUS_ERROR
int run_statement(struct Context *ctx, struct Object **errptr, struct AstNode *stmt);

#endif    // RUN_H
