#ifndef AST_H
#define AST_H

#include <stddef.h>
#include "unicode.h"
#include "tokenizer.h"   // IWYU pragma: keep

struct AstNode {
	char kind;
	size_t lineno;    // 0 for expressions
	void *info;
};

// expressions

// UnicodeString is very simple, see its definition
#define AstStrInfo UnicodeString    // lol
#define AST_STR '"'

struct AstIntInfo { char *valstr; };
#define AST_INT '1'

// array and block infos are the same for less copy/paste boilerplate
// items are statements in the block or elements of the list
struct AstArrayOrBlockInfo { struct AstNode **items; size_t nitems; };
#define AST_ARRAY '['
#define AST_BLOCK '{'

struct AstGetVarInfo { struct UnicodeString varname; };
#define AST_GETVAR 'x'

// name is the name of the attribute or method
struct AstGetAttrOrMethodInfo { struct AstNode *obj; struct UnicodeString name; };
#define AST_GETATTR '.'
#define AST_GETMETHOD ':'


// statements
struct AstCreateOrSetVarInfo { struct UnicodeString varname; struct AstNode *val; };
#define AST_CREATEVAR 'v'
#define AST_SETVAR '='

struct AstSetAttrInfo { struct AstNode *obj; struct UnicodeString attr; struct AstNode *val; };
#define AST_SETATTR '*'    // many other characters are already used, this one isn't

// expressions that can also be statements
struct AstCallInfo { struct AstNode *func; struct AstNode **args; size_t nargs; };
#define AST_CALL '('


// free any AstNode pointer properly
void astnode_free(struct AstNode *node);

// make a recursive copy of an AstNode
struct AstNode *astnode_copy(struct AstNode *node);

// TODO: make parse_expression() static
struct AstNode *parse_expression(struct Token **curtok);
struct AstNode *parse_statement(struct Token **curtok);


#endif    // AST_H
