// some assert()s in this file have a comment like 'TODO: report error'
// it means that invalid syntax causes that assert() to fail

// FIXME: nomemerr handling

#include "ast.h"
#include <assert.h>
#include <stdlib.h>
#include "common.h"
#include "interpreter.h"
#include "objectsystem.h"
#include "tokenizer.h"
#include "unicode.h"
#include "objects/array.h"
#include "objects/classobject.h"
#include "objects/errors.h"
#include "objects/integer.h"
#include "objects/string.h"

static void astnode_foreachref(struct Object *node, void *cbdata, classobject_foreachrefcb cb)
{
	struct AstNodeData *data = node->data;
	size_t i;
	switch (data->kind) {
#define info_as(X) ((struct X *) data->info)
	case AST_ARRAY:
	case AST_BLOCK:
		cb(info_as(AstArrayOrBlockInfo), cbdata);
		break;
	case AST_GETATTR:
	case AST_GETMETHOD:
		cb(info_as(AstGetAttrOrMethodInfo)->objnode, cbdata);
		break;
	case AST_CREATEVAR:
	case AST_SETVAR:
		cb(info_as(AstCreateOrSetVarInfo)->valnode, cbdata);
		break;
	case AST_SETATTR:
		cb(info_as(AstSetAttrInfo)->objnode, cbdata);
		cb(info_as(AstSetAttrInfo)->valnode, cbdata);
		break;
	case AST_CALL:
		cb(info_as(AstCallInfo)->funcnode, cbdata);
		for (i = 0; i < info_as(AstCallInfo)->nargs; i++)
			cb(info_as(AstCallInfo)->argnodes[i], cbdata);
		break;
	case AST_INT:
	case AST_STR:
		cb(info_as(AstIntOrStrInfo), cbdata);
		break;
	case AST_GETVAR:
		// do nothing
		break;
#undef info_as
	default:
		assert(0);      // unknown kind
	}
}

static void astnode_destructor(struct Object *node)
{
	struct AstNodeData *data = node->data;
	switch (data->kind) {
#define info_as(X) ((struct X *) data->info)
	case AST_GETVAR:
		free(info_as(AstGetVarInfo)->varname.val);
		free(data->info);
		break;
	case AST_GETATTR:
	case AST_GETMETHOD:
		free(info_as(AstGetAttrOrMethodInfo)->name.val);
		free(data->info);
		break;
	case AST_CREATEVAR:
	case AST_SETVAR:
		free(info_as(AstCreateOrSetVarInfo)->varname.val);
		free(data->info);
		break;
	case AST_SETATTR:
		free(info_as(AstSetAttrInfo)->attr.val);
		free(data->info);
		break;
	case AST_CALL:
		free(info_as(AstCallInfo)->argnodes);
		free(data->info);
		break;
	case AST_INT:
	case AST_STR:
	case AST_ARRAY:
	case AST_BLOCK:
		// do nothing
		break;
#undef info_as
	default:
		assert(0);  // unknown kind
	}

	free(data);
}

struct Object *astnode_createclass(struct Interpreter *interp)
{
	// the 1 means that AstNode instances may have attributes
	// TODO: add at least kind and lineno attributes to the nodes?
	return classobject_new(interp, "AstNode", interp->builtins.objectclass, 1, astnode_foreachref);
}


struct Object *ast_new_statement(struct Interpreter *interp, char kind, size_t lineno, void *info)
{
	struct AstNodeData *data = malloc(sizeof(struct AstNodeData));
	if (!data) {
		errorobject_setnomem(interp);
		return NULL;
	}
	data->kind = kind;
	data->lineno = lineno;
	data->info = info;

	struct Object *obj = classobject_newinstance(interp, interp->builtins.astnodeclass, data, astnode_destructor);
	if (!obj) {
		free(data);
		return NULL;
	}
	return obj;
}


// remember to change this if you add more expressions!
// this never fails
static int expression_coming_up(struct Token *curtok)
{
	if (!curtok)
		return 0;
	if (curtok->kind == TOKEN_STR || curtok->kind == TOKEN_INT || curtok->kind == TOKEN_ID)
		return 1;
	if (curtok->kind == TOKEN_OP)
		return curtok->str.val[0] == '(' ||
			curtok->str.val[0] == '{' ||
			curtok->str.val[0] == '[';
	return 0;
}


// all parse_blahblah() functions RETURN A NEW REFERENCE or NULL on error


// "asd"
static struct Object *parse_string(struct Interpreter *interp, struct Token **curtok)
{
	// these should be checked by the caller
	assert(*curtok);
	assert((*curtok)->kind == TOKEN_STR);

	// remove " from both ends
	// TODO: do something more? e.g. \n, \t
	struct UnicodeString ustr;
	ustr.len = (*curtok)->str.len - 2;
	ustr.val = (*curtok)->str.val + 1;

	struct Object *info = stringobject_newfromustr(interp, ustr);
	if (!info)
		return NULL;

	struct Object *res = ast_new_expression(interp, AST_STR, info);
	if (!res) {
		OBJECT_DECREF(interp, info);
		return NULL;
	}

	*curtok = (*curtok)->next;
	return res;
}


// 123, -456
static struct Object *parse_int(struct Interpreter *interp, struct Token **curtok)
{
	assert(*curtok);
	assert((*curtok)->kind == TOKEN_INT);

	struct Object *info = integerobject_newfromustr(interp, (*curtok)->str);
	if (!info)
		return NULL;

	struct Object *res = ast_new_expression(interp, AST_INT, info);
	if(!res) {
		OBJECT_DECREF(interp, info);
		return NULL;
	}

	*curtok = (*curtok)->next;
	return res;
}


// x
static struct Object *parse_getvar(struct Interpreter *interp, struct Token **curtok)
{
	assert(*curtok);
	assert((*curtok)->kind == TOKEN_ID);

	struct AstGetVarInfo *info = malloc(sizeof(struct AstGetVarInfo));
	if (!info)
		return NULL;

	if (unicodestring_copyinto((*curtok)->str, &(info->varname)) == STATUS_NOMEM) {
		free(info);
		return NULL;
	}

	struct Object *res = ast_new_expression(interp, AST_GETVAR, info);
	if (!res) {
		free(info->varname.val);
		free(info);
		return NULL;
	}

	*curtok = (*curtok)->next;
	return res;
}


// because i'm lazy, wouldn't be hard to fix
#define MAX_ARGS 100

// func arg1 arg2;
// this takes the function as an already-parsed argument
// this way parse_statement() knows when this should be called
static struct Object *parse_call(struct Interpreter *interp, struct Token **curtok, struct Object *funcnode)
{
	size_t lineno = (*curtok)->lineno;

	struct AstCallInfo *callinfo = malloc(sizeof(struct AstCallInfo));
	if (!callinfo)
		return NULL;

	callinfo->argnodes = malloc(sizeof(struct Object *) * MAX_ARGS);
	if (!callinfo->argnodes) {
		free(callinfo);
		return NULL;
	}

	callinfo->funcnode = funcnode;
	OBJECT_INCREF(interp, funcnode);

	for (callinfo->nargs = 0; expression_coming_up(*curtok) && callinfo->nargs < MAX_ARGS; callinfo->nargs++) {
		struct Object *arg = ast_parse_expression(interp, curtok);
		if(!arg) {
			for(size_t i=0; i < callinfo->nargs; i++)
				OBJECT_DECREF(interp, callinfo->argnodes[i]);
			OBJECT_DECREF(interp, funcnode);
			free(callinfo->argnodes);
			free(callinfo);
			return NULL;
		}

		// don't incref or decref arg, ast_parse_expression() returned a reference
		callinfo->argnodes[callinfo->nargs] = arg;
	}

	// TODO: report error "more than MAX_ARGS arguments"
	assert(!expression_coming_up(*curtok));

	// this can't fail because this is freeing memory, not allocating more
	callinfo->argnodes = realloc(callinfo->argnodes, sizeof(struct Object *) * callinfo->nargs);
	if (callinfo->nargs)       // 0 bytes of memory *MAY* be represented as NULL
		assert(callinfo->argnodes);

	struct Object *res = ast_new_statement(interp, AST_CALL, lineno, callinfo);
	if (!res) {
		for(size_t i=0; i < callinfo->nargs; i++)
			OBJECT_DECREF(interp, callinfo->argnodes[i]);
		OBJECT_DECREF(interp, funcnode);
		free(callinfo->argnodes);
		free(callinfo);
		return NULL;
	}
	return res;
}
#undef MAX_ARGS

// "arg1 `func` arg2"
static struct Object *parse_infix_call(struct Interpreter *interp, struct Token **curtok, struct Object *arg1)
{
	size_t lineno = (*curtok)->lineno;

	// this SHOULD be checked by ast_parse_expression()
	assert((*curtok)->str.len == 1 && (*curtok)->str.val[0] == '`');
	*curtok = (*curtok)->next;

	struct Object *func = ast_parse_expression(interp, curtok);
	if (!func)
		return NULL;

	if ((*curtok)->str.len != 1 || (*curtok)->str.val[0] != '`') {
		// TODO: report error "expected another `"
		assert(0);
	}
	*curtok = (*curtok)->next;

	struct Object *arg2 = ast_parse_expression(interp, curtok);
	if (!arg2) {
		OBJECT_DECREF(interp, func);
		return NULL;
	}

	struct AstCallInfo *callinfo = malloc(sizeof(struct AstCallInfo));
	if (!callinfo) {
		OBJECT_DECREF(interp, arg2);
		OBJECT_DECREF(interp, func);
		return NULL;
	}
	callinfo->funcnode = func;
	callinfo->nargs = 2;

	if (!(callinfo->argnodes = malloc(sizeof(struct Object *) * 2))) {
		free(callinfo);
		OBJECT_DECREF(interp, arg2);
		OBJECT_DECREF(interp, func);
		return NULL;
	}
	callinfo->argnodes[0] = arg1;
	callinfo->argnodes[1] = arg2;

	struct Object *res = ast_new_statement(interp, AST_CALL, lineno, callinfo);
	if (!res) {
		free(callinfo->argnodes);
		free(callinfo);
		OBJECT_DECREF(interp, arg2);
		OBJECT_DECREF(interp, func);
		return NULL;
	}

	// this function is already holding a reference to arg2 and func, but not arg1
	OBJECT_INCREF(interp, arg1);
	return res;
}


// (func arg1 arg2) or (arg1 `func` arg2)
static struct Object *parse_call_expression(struct Interpreter *interp, struct Token **curtok)
{
	// this SHOULD be checked by ast_parse_expression()
	assert((*curtok)->str.len == 1 && (*curtok)->str.val[0] == '(');
	*curtok = (*curtok)->next;

	struct Object *first = ast_parse_expression(interp, curtok);
	if (!first)
		return NULL;

	struct Object *res;
	if ((*curtok)->kind == TOKEN_OP && (*curtok)->str.len == 1 && (*curtok)->str.val[0] == '`')
		res = parse_infix_call(interp, curtok, first);
	else
		res = parse_call(interp, curtok, first);
	OBJECT_DECREF(interp, first);
	if (!res)
		return NULL;

	// TODO: report error "missing ')'"
	assert((*curtok)->str.len == 1 && (*curtok)->str.val[0] == ')');
	*curtok = (*curtok)->next;

	return res;
}


// [a b c]
static struct Object *parse_array(struct Interpreter *interp, struct Token **curtok)
{
	assert(*curtok);
	assert((*curtok)->kind == TOKEN_OP);
	assert((*curtok)->str.len == 1);
	assert((*curtok)->str.val[0] == '[');
	*curtok = (*curtok)->next;
	assert(*curtok);    // TODO: report error "unexpected end of file"

	struct Object *elements = arrayobject_newempty(interp);
	if (!elements)
		return NULL;

	while ((*curtok) && !((*curtok)->kind == TOKEN_OP && (*curtok)->str.len == 1 && (*curtok)->str.val[0] == ']')) {
		struct Object *elem = ast_parse_expression(interp, curtok);
		if(!elem) {
			OBJECT_DECREF(interp, elements);
			return NULL;
		}
		int pushret = arrayobject_push(interp, elements, elem );
		OBJECT_DECREF(interp, elem);
		if (pushret == STATUS_ERROR) {
			OBJECT_DECREF(interp, elements);
			return NULL;
		}
	}

	assert(*curtok);   // TODO: report error "unexpected end of file"
	assert((*curtok)->str.len == 1 && (*curtok)->str.val[0] == ']');
	*curtok = (*curtok)->next;   // skip ']'

	struct Object *res = ast_new_expression(interp, AST_ARRAY, elements);
	if (!res) {
		OBJECT_DECREF(interp, elements);
		return NULL;
	}
	return res;
}

// { ... }
struct Object *parse_block(struct Interpreter *interp, struct Token **curtok)
{
	assert(*curtok);
	assert((*curtok)->kind == TOKEN_OP);
	assert((*curtok)->str.len == 1);
	assert((*curtok)->str.val[0] == '{');
	*curtok = (*curtok)->next;
	assert(*curtok);    // TODO: report error "unexpected end of file"

	struct Object *statements = arrayobject_newempty(interp);
	if (!statements)
		return NULL;

	// TODO: { expr } should be same as { return expr; }
	while ((*curtok) && !((*curtok)->kind == TOKEN_OP && (*curtok)->str.len == 1 && (*curtok)->str.val[0] == '}')) {
		struct Object *stmt = ast_parse_statement(interp, curtok);
		if (!stmt) {
			OBJECT_DECREF(interp, statements);
			return NULL;
		}
		int pushret = arrayobject_push(interp, statements, stmt);
		OBJECT_DECREF(interp, stmt);
		if (pushret == STATUS_ERROR) {
			OBJECT_DECREF(interp, statements);
			return NULL;
		}
	}

	assert(*curtok);   // TODO: report error "unexpected end of file"
	assert((*curtok)->str.len == 1 && (*curtok)->str.val[0] == '}');
	*curtok = (*curtok)->next;   // skip ']'

	struct Object *res = ast_new_expression(interp, AST_BLOCK, statements);
	if (!res) {
		OBJECT_DECREF(interp, statements);
		return NULL;
	}
	return res;
}


struct Object *ast_parse_expression(struct Interpreter *interp, struct Token **curtok)
{
	struct Object *res;
	switch ((*curtok)->kind) {
	case TOKEN_STR:
		res = parse_string(interp, curtok);
		break;
	case TOKEN_INT:
		res = parse_int(interp, curtok);
		break;
	case TOKEN_ID:
		res = parse_getvar(interp, curtok);
		break;
	case TOKEN_OP:
		if ((*curtok)->str.len == 1) {
			if ((*curtok)->str.val[0] == '[') {
				res = parse_array(interp, curtok);
				break;
			}
			if ((*curtok)->str.val[0] == '{') {
				res = parse_block(interp, curtok);
				break;
			}
			if ((*curtok)->str.val[0] == '(') {
				res = parse_call_expression(interp, curtok);
				break;
			}
		}
	default:
		// TODO: report error "expected this, that or those, got '%s'"
		assert(0);
	}
	if (!res)
		return NULL;

	// attributes and methods
	while ((*curtok) && (*curtok)->kind == TOKEN_OP && (
			((*curtok)->str.len == 1 && (*curtok)->str.val[0] == '.') ||
			((*curtok)->str.len == 2 && (*curtok)->str.val[0] == ':' && (*curtok)->str.val[1] == ':'))) {
		char astkind = (*curtok)->str.len == 1 ? AST_GETATTR : AST_GETMETHOD;
		*curtok = (*curtok)->next;   // skip '.' or '::'
		assert((*curtok));     // TODO: report error "expected an attribute name, but the file ended"
		assert((*curtok)->kind == TOKEN_ID);   // TODO: report error "invalid attribute name 'bla bla'"

		struct AstGetAttrOrMethodInfo *gaminfo = malloc(sizeof(struct AstGetAttrOrMethodInfo));
		if(!gaminfo) {
			// TODO: set no mem error
			OBJECT_DECREF(interp, res);
			return NULL;
		}

		// no need to incref, this function is already holding a reference to res
		gaminfo->objnode = res;

		if (unicodestring_copyinto((*curtok)->str, &(gaminfo->name)) != STATUS_OK) {
			// TODO: set no mem error
			OBJECT_DECREF(interp, res);
			free(gaminfo);
			return NULL;
		}
		*curtok = (*curtok)->next;

		struct Object *gam = ast_new_expression(interp, astkind, gaminfo);
		if(!gam) {
			free(gaminfo->name.val);
			free(gaminfo);
			OBJECT_DECREF(interp, res);
			return NULL;
		}
		res = gam;
	}

	return res;
}


// var x = y;
static struct Object *parse_var_statement(struct Interpreter *interp, struct Token **curtok)
{
	// parse_statement() has checked (*curtok)->kind
	// TODO: report error?
	size_t lineno = (*curtok)->lineno;
	assert((*curtok)->str.len == 3);
	assert((*curtok)->str.val[0] == 'v');
	assert((*curtok)->str.val[1] == 'a');
	assert((*curtok)->str.val[2] == 'r');
	*curtok = (*curtok)->next;

	// TODO: report error
	assert((*curtok)->kind == TOKEN_ID);

	struct UnicodeString varname;
	if (unicodestring_copyinto((*curtok)->str, &varname) == STATUS_NOMEM) {
		// TODO: report no mem error
		return NULL;
	}
	*curtok = (*curtok)->next;

	// TODO: should 'var x;' set x to null? or just be forbidden?
	assert((*curtok)->kind == TOKEN_OP);
	assert((*curtok)->str.len == 1);
	assert((*curtok)->str.val[0] == '=');
	*curtok = (*curtok)->next;

	struct Object *value = ast_parse_expression(interp, curtok);
	if (!value) {
		free(varname.val);
		return NULL;
	}

	struct AstCreateOrSetVarInfo *info = malloc(sizeof(struct AstCreateOrSetVarInfo));
	if (!info) {
		// TODO: set no mem error
		OBJECT_DECREF(interp, value);
		free(varname.val);
		return NULL;
	}
	info->varname = varname;
	info->valnode = value;

	struct Object *res = ast_new_statement(interp, AST_CREATEVAR, lineno, info);
	if (!res) {
		// TODO: set no mem error
		free(info);
		OBJECT_DECREF(interp, value);
		free(varname.val);
		return NULL;
	}
	return res;
}

// x = y;
static struct Object *parse_assignment(struct Interpreter *interp, struct Token **curtok, struct Object *lhs)
{
	struct AstNodeData *lhsdata = lhs->data;
	if (lhsdata->kind != AST_GETVAR && lhsdata->kind != AST_GETATTR) {
		// TODO: report an error e.g. like this:
		//   the x of 'x = y;' must be a variable name or an attribute
		assert(0);
	}

	// these should be checked by the caller
	assert((*curtok)->str.len == 1);
	assert((*curtok)->str.val[0] == '=');
	*curtok = (*curtok)->next;

	struct Object *rhs = ast_parse_expression(interp, curtok);
	if (!rhs)
		return NULL;

	if (lhsdata->kind == AST_GETVAR) {
		struct AstGetVarInfo *lhsinfo = lhsdata->info;
		struct AstCreateOrSetVarInfo *info = malloc(sizeof(struct AstCreateOrSetVarInfo));
		if (!info)
			goto error;

		if (unicodestring_copyinto(lhsinfo->varname, &(info->varname)) == STATUS_NOMEM) {
			errorobject_setnomem(interp);
			free(info);
			goto error;
		}
		info->valnode = rhs;

		struct Object *result = ast_new_statement(interp, AST_SETVAR, lhsdata->lineno, info);
		if (!result) {
			free(info->varname.val);
			free(info);
			goto error;
		}
		return result;
	} else {
		assert(lhsdata->kind == AST_GETATTR);
		struct AstGetAttrOrMethodInfo *lhsinfo = lhsdata->info;
		struct AstSetAttrInfo *info = malloc(sizeof(struct AstSetAttrInfo));
		if (!info)
			goto error;

		if (unicodestring_copyinto(lhsinfo->name, &(info->attr)) == STATUS_NOMEM) {
			errorobject_setnomem(interp);
			free(info);
			goto error;
		}
		info->objnode = lhsinfo->objnode;
		OBJECT_INCREF(interp, lhsinfo->objnode);
		info->valnode = rhs;

		struct Object *result = ast_new_statement(interp, AST_SETATTR, lhsdata->lineno, info);
		if (!result) {
			OBJECT_DECREF(interp, lhsinfo->objnode);
			free(info->attr.val);
			free(info);
			goto error;
		}
		return result;
	}

error:
	OBJECT_DECREF(interp, rhs);
	return NULL;
}

struct Object *ast_parse_statement(struct Interpreter *interp, struct Token **curtok)
{
	struct Object *res;
	if ((*curtok)->kind == TOKEN_KEYWORD) {
		// var is currently the only keyword
		res = parse_var_statement(interp, curtok);
	} else {
		// a function call or an assignment
		struct Object *first = ast_parse_expression(interp, curtok);
		if (!first)
			return NULL;

		if ((*curtok)->kind == TOKEN_OP && (*curtok)->str.len == 1 && (*curtok)->str.val[0] == '=')
			res = parse_assignment(interp, curtok, first);
		else if ((*curtok)->kind == TOKEN_OP && (*curtok)->str.len == 1 && (*curtok)->str.val[0] == '`')
			res = parse_infix_call(interp, curtok, first);
		else
			res = parse_call(interp, curtok, first);
		OBJECT_DECREF(interp, first);
		if (!res)
			return NULL;
	}

	// TODO: report error "missing ';'"
	assert((*curtok)->kind == TOKEN_OP);
	assert((*curtok)->str.len == 1);
	assert((*curtok)->str.val[0] == ';');
	*curtok = (*curtok)->next;
	return res;
}
