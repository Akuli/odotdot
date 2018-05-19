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
		for (i = 0; i < info_as(AstArrayOrBlockInfo)->nitems; i++)
			cb(info_as(AstArrayOrBlockInfo)->itemnodes[i], cbdata);
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
	case AST_ARRAY:
	case AST_BLOCK:
		// foreachref has taken care of decreffing the items
		free(info_as(AstArrayOrBlockInfo)->itemnodes);
		free(data->info);
		break;
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
		// do nothing
		break;
#undef info_as
	default:
		assert(0);  // unknown kind
	}

	free(data);
}

struct Object *astnode_createclass(struct Interpreter *interp, struct Object **errptr)
{
	// the 1 means that AstNode instances may have attributes
	// TODO: add at least kind and lineno attributes to the nodes?
	return classobject_new(interp, errptr, "AstNode", interp->builtins.objectclass, 1, astnode_foreachref);
}


// RETURNS A NEW REFERENCE or NULL on error
struct Object *ast_new_statement(struct Interpreter *interp, struct Object **errptr, char kind, size_t lineno, void *info)
{
	struct AstNodeData *data = malloc(sizeof(struct AstNodeData));
	if (!data) {
		errorobject_setnomem(interp, errptr);
		return NULL;
	}
	data->kind = kind;
	data->lineno = lineno;
	data->info = info;

	struct Object *obj = classobject_newinstance(interp, errptr, interp->builtins.astnodeclass, data, astnode_destructor);
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


static struct Object *parse_string(struct Interpreter *interp, struct Object **errptr, struct Token **curtok)
{
	// these should be checked by the caller
	assert(*curtok);
	assert((*curtok)->kind == TOKEN_STR);

	// remove " from both ends
	// TODO: do something more? e.g. \n, \t
	struct UnicodeString ustr;
	ustr.len = (*curtok)->str.len - 2;
	ustr.val = (*curtok)->str.val + 1;

	struct Object *info = stringobject_newfromustr(interp, errptr, ustr);
	if (!info)
		return NULL;

	struct Object *res = ast_new_expression(interp, errptr, AST_STR, info);
	if (!res) {
		OBJECT_DECREF(interp, info);
		return NULL;
	}

	*curtok = (*curtok)->next;
	return res;
}


static struct Object *parse_int(struct Interpreter *interp, struct Object **errptr, struct Token **curtok)
{
	assert(*curtok);
	assert((*curtok)->kind == TOKEN_INT);

	struct Object *info = integerobject_newfromustr(interp, errptr, (*curtok)->str);
	if (!info)
		return NULL;

	struct Object *res = ast_new_expression(interp, errptr, AST_INT, info);
	if(!res) {
		OBJECT_DECREF(interp, info);
		return NULL;
	}

	*curtok = (*curtok)->next;
	return res;
}


// RETURNS A NEW REFERENCE or NULL on error
static struct Object *parse_getvar(struct Interpreter *interp, struct Object **errptr, struct Token **curtok)
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

	struct Object *res = ast_new_expression(interp, errptr, AST_GETVAR, info);
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

// this takes the function as an already-parsed argument
// this way parse_statement() knows when this should be called
static struct Object *parse_call(struct Interpreter *interp, struct Object **errptr, struct Token **curtok, struct Object *funcnode)
{
	size_t lineno = (*curtok)->lineno;

	// TODO: when should func be free()'d?
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
		struct Object *arg = ast_parse_expression(interp, errptr, curtok);
		if(!arg) {
			for(size_t i=0; i < callinfo->nargs; i++)
				OBJECT_DECREF(interp, callinfo->argnodes[i]);
			OBJECT_DECREF(interp, funcnode);
			free(callinfo->argnodes);
			free(callinfo);
			return NULL;
		}

		// don't incref or decref arg, parse_expression() returned a reference
		callinfo->argnodes[callinfo->nargs] = arg;
	}

	// TODO: report error "more than MAX_ARGS arguments"
	assert(!expression_coming_up(*curtok));

	// this can't fail because this is freeing memory, not allocating more
	callinfo->argnodes = realloc(callinfo->argnodes, sizeof(struct Object *) * callinfo->nargs);
	if (callinfo->nargs)       // 0 bytes of memory *MAY* be represented as NULL
		assert(callinfo->argnodes);

	struct Object *res = ast_new_statement(interp, errptr, AST_CALL, lineno, callinfo);
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


static struct Object *parse_call_expression(struct Interpreter *interp, struct Object **errptr, struct Token **curtok)
{
	// this SHOULD be checked by parse_expression()
	assert((*curtok)->str.len == 1 && (*curtok)->str.val[0] == '(');
	*curtok = (*curtok)->next;

	struct Object *func = ast_parse_expression(interp, errptr, curtok);
	if (!func)
		return NULL;

	struct Object *res = parse_call(interp, errptr, curtok, func);
	OBJECT_DECREF(interp, func);
	if (!res)
		return NULL;

	// TODO: report error "missing ')'"
	assert((*curtok)->str.len == 1 && (*curtok)->str.val[0] == ')');
	*curtok = (*curtok)->next;

	return res;
}


static struct Object *parse_array(struct Interpreter *interp, struct Object **errptr, struct Token **curtok)
{
	assert(*curtok);
	assert((*curtok)->kind == TOKEN_OP);
	assert((*curtok)->str.len == 1);
	assert((*curtok)->str.val[0] == '[');
	*curtok = (*curtok)->next;
	assert(*curtok);    // TODO: report error "unexpected end of file"

	// i thought about looping over the elements twice and then doing just one
	// malloc, but parsing the list items means many mallocs so this is probably
	// faster
	size_t nelems = 0;
	size_t nallocated = 0;
	struct Object **elems = NULL;

	while ((*curtok) && !((*curtok)->kind == TOKEN_OP && (*curtok)->str.len == 1 && (*curtok)->str.val[0] == ']')) {
		struct Object *elem = ast_parse_expression(interp, errptr, curtok);
		if(!elem)
			goto error;

		if (nelems+1 > nallocated) {
			size_t newnallocated = nallocated + 64;   // TODO: is 64 the best possible size?
			void *ptr = realloc(elems, sizeof(struct Object *) * newnallocated);
			if (!ptr) {
				// TODO: set no mem error
				goto error;
			}
			elems = ptr;
			nallocated = newnallocated;
		}
		elems[nelems] = elem;
		nelems++;
	}

	assert(*curtok);   // TODO: report error "unexpected end of file"
	assert((*curtok)->str.len == 1 && (*curtok)->str.val[0] == ']');
	*curtok = (*curtok)->next;   // skip ']'

	// this can't fail because it doesn't actually allocate more, it frees allocated mem
	elems = realloc(elems, sizeof(struct Object *) * nelems);
	assert(elems);
	nallocated = nelems;

	struct AstArrayOrBlockInfo *info = malloc(sizeof(struct AstArrayOrBlockInfo));
	if (!info)
		goto error;
	info->itemnodes = elems;
	info->nitems = nelems;

	struct Object *res = ast_new_expression(interp, errptr, AST_ARRAY, info);
	if (!res) {
		free(info);
		goto error;
	}
	return res;

error:
	for (size_t i=0; i < nelems; i++)
		OBJECT_DECREF(interp, elems[i]);
	if (elems)
		free(elems);
	return NULL;
}


struct Object *ast_parse_expression(struct Interpreter *interp, struct Object **errptr, struct Token **curtok)
{
	struct Object *res;
	switch ((*curtok)->kind) {
	case TOKEN_STR:
		res = parse_string(interp, errptr, curtok);
		break;
	case TOKEN_INT:
		res = parse_int(interp, errptr, curtok);
		break;
	case TOKEN_ID:
		res = parse_getvar(interp, errptr, curtok);
		break;
	case TOKEN_OP:
		if ((*curtok)->str.len == 1) {
			if ((*curtok)->str.val[0] == '[') {
				res = parse_array(interp, errptr, curtok);
				break;
			}
			if ((*curtok)->str.val[0] == '(') {
				res = parse_call_expression(interp, errptr, curtok);
				break;
			}
		}
	default:
		// TODO: report error "expected this, that or those, got '%s'"
		assert(0);
	}
	if (!res)       // no mem
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

		struct Object *gam = ast_new_expression(interp, errptr, astkind, gaminfo);
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


static struct Object *parse_var_statement(struct Interpreter *interp, struct Object **errptr, struct Token **curtok)
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

	struct Object *value = ast_parse_expression(interp, errptr, curtok);
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

	struct Object *res = ast_new_statement(interp, errptr, AST_CREATEVAR, lineno, info);
	if (!res) {
		// TODO: set no mem error
		free(info);
		OBJECT_DECREF(interp, value);
		free(varname.val);
		return NULL;
	}
	return res;
}

struct Object *ast_parse_statement(struct Interpreter *interp, struct Object **errptr, struct Token **curtok)
{
	struct Object *res;
	if ((*curtok)->kind == TOKEN_KEYWORD) {
		// var is currently the only keyword
		res = parse_var_statement(interp, errptr, curtok);
	} else {
		// assume it's a function call
		struct Object *funcnode = ast_parse_expression(interp, errptr, curtok);
		if (!funcnode)
			return NULL;

		res = parse_call(interp, errptr, curtok, funcnode);
		OBJECT_DECREF(interp, funcnode);
		if (!res)
			return NULL;
		assert(!expression_coming_up(*curtok));   // parse_call() should take care of this
	}

	// TODO: report error "missing ';'"
	assert((*curtok)->kind == TOKEN_OP);
	assert((*curtok)->str.len == 1);
	assert((*curtok)->str.val[0] == ';');
	*curtok = (*curtok)->next;
	return res;
}
