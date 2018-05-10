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


// expressions

// UnicodeString is very simple, see its definition
#define AstStrInfo UnicodeString    // lol
#define AST_STR '"'

// TODO: replace this with integer objects?
struct AstIntInfo { char *valstr; };
#define AST_INT '1'

// array and block infos are the same for less copy/paste boilerplate
// itemnodes are statements in the block or elements of the list
struct AstArrayOrBlockInfo { struct Object **itemnodes; size_t nitems; };
#define AST_ARRAY '['
#define AST_BLOCK '{'

// TODO: replace this with UnicodeString directly, like AstStrInfo?
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
