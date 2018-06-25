#ifndef OBJECTS_ASTNODE_H
#define OBJECTS_ASTNODE_H

#include <stddef.h>
#include "../interpreter.h"    // IWYU pragma: keep
#include "../objectsystem.h"   // IWYU pragma: keep
#include "../unicode.h"
#include "../operator.h"

struct AstNodeObjectData {
	char kind;
	char *filename;
	size_t lineno;   // starts at 1
	void *info;
};

// RETURNS A NEW REFERENCE or NULL on error
struct Object *astnodeobject_createclass(struct Interpreter *interp);

// for creating ast in things like tests
// RETURNS A NEW REFERENCE or NULL on error
struct Object *astnodeobject_new(struct Interpreter *interp, char kind, char *filename, size_t lineno, void *info);

// TODO: expressions should also have line number information
#define ast_new_expression(interp, kind, info) ast_new_statement((interp), (kind), 0, (info))


// expressions
// TODO: get rid of the //lol macros

// the info is a String or Integer object
#define AstIntOrStrInfo Object    //lol
#define AST_INT '1'
#define AST_STR '"'

// array and block infos are Array objects of ast nodes
#define AstArrayOrBlockInfo Object    //lol
#define AST_ARRAY '['
#define AST_BLOCK '{'

// TODO: replace this with UnicodeString, kinda like AstIntOrStringInfo?
struct AstGetVarInfo { struct UnicodeString varname; };
#define AST_GETVAR 'x'

struct AstGetAttrInfo { struct Object *objnode; struct UnicodeString name; };
#define AST_GETATTR '.'


// statements

struct AstCreateOrSetVarInfo { struct UnicodeString varname; struct Object *valnode; };
#define AST_CREATEVAR 'v'
#define AST_SETVAR '='

struct AstSetAttrInfo { struct Object *objnode; struct UnicodeString attr; struct Object *valnode; };
#define AST_SETATTR '*'    // many other characters are already used, this one isn't


// expressions that can also be statements

// args is an Array object that contains AstNodes
// opts is a Mapping with String keys and AstNode values
struct AstCallInfo { struct Object *funcnode; struct Object *args; struct Object *opts; };
#define AST_CALL '('

// lhs and rhs are AstNodes
struct AstOpCallInfo { enum Operator op; struct Object *lhs; struct Object *rhs; };
#define AST_OPCALL '+'


#endif     // OBJECTS_ASTNODE_H
