#ifndef AST_H
#define AST_H

#include <stddef.h>
#include "tokenizer.h"   // needed for struct Token, but iwyu doesn't get it
#include "utf8.h"

struct AstNode {
	char kind;
	size_t lineno;    // 0 for expressions
	void *info;
};

// expressions
struct AstStrInfo { unicode_t *val; size_t len; };
#define AST_STR '"'
struct AstIntInfo { char *valstr; };
#define AST_INT '1'
// array and block infos are the same for less copy/paste boilerplate
// items are statements in the block or elements of the list
struct AstArrayOrBlockInfo { struct AstNode **items; size_t nitems; };
#define AST_ARRAY '['
#define AST_BLOCK '{'
struct AstGetVarInfo { struct AstStrInfo *varname; };
#define AST_GETVAR 'x'
struct AstGetAttrInfo { struct AstNode *obj; struct AstStrInfo *attr; };
#define AST_GETATTR '.'

// statements
struct AstCreateOrSetVarInfo { struct AstStrInfo *varname; struct AstNode *val; };
#define AST_CREATEVAR 'v'
#define AST_SETVAR '='
struct AstSetAttrInfo { struct AstNode *obj; struct AstStrInfo *attr; struct AstNode *val; };
#define AST_SETATTR ':'    // '.' and '=' are already used

// expressions that can also be statements
struct AstCallInfo { struct AstNode *func; struct AstNode **args; size_t nargs; };
#define AST_CALL '('


// free any AstNode pointer properly
void astnode_free(struct AstNode *node);

// make a recursive copy of an AstNode
struct AstNode *astnode_copy(struct AstNode *node);

// TODO: make this static
struct AstNode *parse_expression(struct Token **curtok);


#endif    // AST_H
