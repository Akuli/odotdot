#ifndef AST_H
#define AST_H

struct Interpreter;
struct Token;

// TODO: make parse_expression() static?
// these RETURN A NEW REFERENCE or NULL on error
struct Object *parse_expression(struct Interpreter *interp, char *filename, struct Token **curtok);
struct Object *parse_statement(struct Interpreter *interp, char *filename, struct Token **curtok);

#endif    // AST_H
