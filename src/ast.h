#ifndef AST_H
#define AST_H

#include <stddef.h>
#include "interpreter.h"    // IWYU pragma: keep
#include "objectsystem.h"   // IWYU pragma: keep
#include "tokenizer.h"      // IWYU pragma: keep
#include "unicode.h"

struct AstNodeData {
	char kind;
	size_t lineno;    // 0 for expressions
	void *info;
};

// RETURNS A NEW REFERENCE or NULL on error
struct Object *astnode_createclass(struct Interpreter *interp, struct Object **errptr);

// for creating ast in things like tests
struct Object *ast_new_statement(struct Interpreter *interp, struct Object **errptr, char kind, size_t lineno, void *info);
#define ast_new_expression(interp, errptr, kind, info) ast_new_statement((interp), (errptr), (kind), 0, (info))


// expressions

// the info is a String or Integer object
#define AstIntOrStrInfo Object    // lol
#define AST_INT '1'
#define AST_STR '"'

// array and block infos are Array objects of ast nodes
#define AstArrayOrBlockInfo Object
#define AST_ARRAY '['
#define AST_BLOCK '{'

// TODO: replace this with UnicodeString, kinda like AstIntOrStringInfo?
struct AstGetVarInfo { struct UnicodeString varname; };
#define AST_GETVAR 'x'

// obj is an ast node object
// name is attribute or method name
struct AstGetAttrOrMethodInfo { struct Object *objnode; struct UnicodeString name; };
#define AST_GETATTR '.'
#define AST_GETMETHOD ':'


// statements

struct AstCreateOrSetVarInfo { struct UnicodeString varname; struct Object *valnode; };
#define AST_CREATEVAR 'v'
#define AST_SETVAR '='

struct AstSetAttrInfo { struct Object *objnode; struct UnicodeString attr; struct Object *valnode; };
#define AST_SETATTR '*'    // many other characters are already used, this one isn't


// expressions that can also be statements

struct AstCallInfo { struct Object *funcnode; struct Object **argnodes; size_t nargs; };
#define AST_CALL '('


// TODO: make parse_expression() static?
// these RETURN A NEW REFERENCE or NULL on error
struct Object *ast_parse_expression(struct Interpreter *interp, struct Object **errptr, struct Token **curtok);
struct Object *ast_parse_statement(struct Interpreter *interp, struct Object **errptr, struct Token **curtok);


#endif    // AST_H
