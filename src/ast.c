// some assert()s in this file have a comment like 'TODO: report error'
// it means that invalid syntax causes that assert() to fail

#include "ast.h"
#include <assert.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include "common.h"
#include "unicode.h"
#include "tokenizer.h"

static void free_info(char kind, void *info)
{
	switch (kind) {
#define info_as(X) ((struct X *) info)
	case AST_STR:
		free(info_as(AstStrInfo)->val);
		break;
	case AST_INT:
		free(info_as(AstIntInfo)->valstr);
		break;
	case AST_ARRAY:
	case AST_BLOCK:
		for (size_t i=0; i < info_as(AstArrayOrBlockInfo)->nitems; i++)
			astnode_free(info_as(AstArrayOrBlockInfo)->items[i]);
		free(info_as(AstArrayOrBlockInfo)->items);
		break;
	case AST_GETVAR:
		free(info_as(AstGetVarInfo)->varname.val);
		break;
	case AST_GETATTR:
		astnode_free(info_as(AstGetAttrInfo)->obj);
		free(info_as(AstGetAttrInfo)->attr.val);
		break;
	case AST_CREATEVAR:
	case AST_SETVAR:
		free(info_as(AstCreateOrSetVarInfo)->varname.val);
		astnode_free(info_as(AstCreateOrSetVarInfo)->val);
		break;
	case AST_SETATTR:
		astnode_free(info_as(AstSetAttrInfo)->obj);
		free(info_as(AstSetAttrInfo)->attr.val);
		astnode_free(info_as(AstSetAttrInfo)->val);
		break;
	case AST_CALL:
		astnode_free(info_as(AstCallInfo)->func);
		for (size_t i=0; i < info_as(AstCallInfo)->nargs; i++)
			astnode_free(info_as(AstCallInfo)->args[i]);
		free(info_as(AstCallInfo)->args);
		break;
#undef info_as
	default:
		assert(0);  // unknown kind
	}
	free(info);
}

void astnode_free(struct AstNode *node)
{
	free_info(node->kind, node->info);
	free(node);
}


static void *copy_info(char kind, void *info)
{
	// can't use switch because each of these defines a res with a different type
#define info_as(X) ((struct X *) info)
	if(kind == AST_STR) {
		// AstStrInfo is a macro for UnicodeString
		return unicodestring_copy(*info_as(UnicodeString));
	}
	if (kind == AST_INT) {
		struct AstIntInfo *res = malloc(sizeof(struct AstIntInfo));
		if(!res)
			return NULL;
		res->valstr = malloc(strlen(info_as(AstIntInfo)->valstr)+1);
		if(!res->valstr) {
			free(res);
			return NULL;
		}
		strcpy(res->valstr, info_as(AstIntInfo)->valstr);
		return res;
	}
	if(kind == AST_ARRAY|| kind == AST_BLOCK) {
		struct AstArrayOrBlockInfo *res = malloc(sizeof(struct AstArrayOrBlockInfo));
		if(!res)
			return NULL;
		res->nitems = info_as(AstArrayOrBlockInfo)->nitems;
		res->items = malloc(sizeof(struct AstNode*) * res->nitems);
		if(!res->items){
			free(res);
			return NULL;
		}
		for (size_t i=0; i < res->nitems; i++) {
			struct AstNode *item = astnode_copy(info_as(AstArrayOrBlockInfo)->items[i]);
			if(!item) {
				for(size_t j=0; j<i; j++)
					astnode_free(res->items[j]);
				free(res->items);
				free(res);
				return NULL;
			}
			res->items[i] = item;
		}
		return res;
	}
	if(kind == AST_GETVAR) {
		struct AstGetVarInfo *res = malloc(sizeof(struct AstGetVarInfo));
		if(!res)
			return NULL;
		if (unicodestring_copyinto(info_as(AstGetVarInfo)->varname, &(res->varname)) != STATUS_OK) {
			free(res);
			return NULL;
		}
		return res;
	}
	if (kind == AST_GETATTR) {
		struct AstGetAttrInfo *res = malloc(sizeof(struct AstGetAttrInfo));
		if(!res)
			return NULL;
		if (unicodestring_copyinto(info_as(AstGetAttrInfo)->attr, &(res->attr)) != STATUS_OK) {
			free(res);
			return NULL;
		}
		res->obj = astnode_copy(info_as(AstGetAttrInfo)->obj);
		if(!(res->obj)) {
			free(res->attr.val);
			free(res);
			return NULL;
		}
		return res;
	}
	if (kind == AST_CREATEVAR || kind == AST_SETVAR) {
		struct AstCreateOrSetVarInfo *res = malloc(sizeof(struct AstCreateOrSetVarInfo));
		if (!res)
			return NULL;
		if (unicodestring_copyinto(info_as(AstCreateOrSetVarInfo)->varname, &(res->varname)) != STATUS_OK) {
			free(res);
			return NULL;
		}
		res->val = astnode_copy(info_as(AstCreateOrSetVarInfo)->val);
		if (!res->val) {
			free(res->varname.val);
			free(res);
			return NULL;
		}
		return res;
	}
	if (kind == AST_SETATTR) {
		struct AstSetAttrInfo *res = malloc(sizeof(struct AstSetAttrInfo));
		if(!res)
			return NULL;
		res->obj = astnode_copy(info_as(AstSetAttrInfo)->obj);
		if(!res->obj) {
			free(res);
			return NULL;
		}
		if (unicodestring_copyinto(info_as(AstSetAttrInfo)->attr, &(res->attr)) != STATUS_OK) {
			astnode_free(res->obj);
			free(res);
			return NULL;
		}
		res->val = astnode_copy(info_as(AstSetAttrInfo)->val);
		if(!res->val) {
			free(res->attr.val);
			astnode_free(res->obj);
			free(res);
			return NULL;
		}
		return res;
	}
	if (kind == AST_CALL) {
		struct AstCallInfo *res = malloc(sizeof(struct AstCallInfo));
		if(!res)
			return NULL;
		res->func = astnode_copy(info_as(AstCallInfo)->func);
		if(!(res->func)) {
			astnode_free(res->func);
			free(res);
			return NULL;
		}
		res->nargs = info_as(AstCallInfo)->nargs;
		res->args = malloc(sizeof(struct AstNode*) * res->nargs);
		if(!res->args){
			astnode_free(res->func);
			free(res);
			return NULL;
		}
		for (size_t i=0; i < res->nargs; i++) {
			struct AstNode *arg = astnode_copy(info_as(AstCallInfo)->args[i]);
			if(!arg) {
				for(size_t j=0; j<i; j++)
					astnode_free(res->args[j]);
				free(res->args);
				astnode_free(res->func);
				free(res);
				return NULL;
			}
			res->args[i] = arg;
		}
		return res;
	}
#undef info_as
	assert(0);
}

struct AstNode *astnode_copy(struct AstNode *node) {
	void *resinfo = copy_info(node->kind, node->info);
	if(!resinfo)
		return NULL;

	struct AstNode *res = malloc(sizeof(struct AstNode));
	if(!res) {
		free(resinfo);
		return NULL;
	}
	res->kind = node->kind;
	res->lineno = node->lineno;
	res->info = resinfo;
	return res;
}


static struct AstNode *new_statement(char kind, size_t lineno, void *info)
{
	struct AstNode *obj = malloc(sizeof(struct AstNode));
	if (!obj)
		return NULL;
	obj->kind = kind;
	obj->lineno = lineno;
	obj->info = info;
	return obj;
}
#define new_expression(kind, info) new_statement((kind), 0, (info))


// use with free_info(AST_STR, the_strinfo) in error handling code
struct AstStrInfo *strinfo_from_idtoken(struct Token **curtok)
{
	assert(*curtok);
	assert((*curtok)->kind == TOKEN_ID);
	struct UnicodeString *res = unicodestring_copy((*curtok)->str);
	*curtok = (*curtok)->next;
	return res;   // may be NULL
}


static struct AstNode *parse_string(struct Token **curtok)
{
	assert(*curtok);
	assert((*curtok)->kind == TOKEN_STR);

	// can't use strinfo_from_idtoken because "" must be removed
	struct AstStrInfo *info = malloc(sizeof(struct AstStrInfo));
	if (!info)
		return NULL;

	// remove " from both ends
	// TODO: do something more? e.g. \n, \t
	info->len = (*curtok)->str.len - 2;
	info->val = malloc(sizeof(uint32_t) * info->len);
	if (!(info->val)) {
		free(info);
		return NULL;
	}
	memcpy(info->val, (*curtok)->str.val+1, sizeof(uint32_t) * info->len);

	*curtok = (*curtok)->next;
	struct AstNode *res = new_expression(AST_STR, info);
	if (!res) {
		free(info->val);
		free(info);
		return NULL;
	}
	return res;
}


static struct AstNode *parse_int(struct Token **curtok)
{
	assert(*curtok);
	assert((*curtok)->kind == TOKEN_INT);

	struct AstIntInfo *info = malloc(sizeof(struct AstIntInfo));
	if (!info)
		return NULL;

	info->valstr = malloc((*curtok)->str.len+1);
	if (!(info->valstr)) {
		free(info);
		return NULL;
	}

	// can't use memcpy because (*curtok)->str.val is uint32_t*
	// TODO: should construct the integer object here??
	for (size_t i=0; i < (*curtok)->str.len; i++)
		info->valstr[i] = (char) ((*curtok)->str.val[i]);
	info->valstr[(*curtok)->str.len] = 0;

	*curtok = (*curtok)->next;
	struct AstNode *res = new_expression(AST_INT, info);
	if(!res) {
		free(info->valstr);
		free(info);
		return NULL;
	}
	return res;
}


static struct AstNode *parse_getvar(struct Token **curtok)
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

	struct AstNode *res = new_expression(AST_GETVAR, info);
	if (!res) {
		free(info->varname.val);
		free(info);
		return NULL;
	}

	*curtok = (*curtok)->next;
	return res;
}


static struct AstNode *parse_array(struct Token **curtok)
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
	struct AstNode **elems = NULL;

	while ((*curtok) && !((*curtok)->kind == TOKEN_OP && (*curtok)->str.len == 1 && (*curtok)->str.val[0] == ']')) {
		struct AstNode *elem = parse_expression(curtok);
		if(!elem)
			goto error;

		if (nelems+1 > nallocated) {
			size_t newnallocated = nallocated + 64;   // TODO: is 64 the best possible size?
			void *ptr = realloc(elems, sizeof(struct AstNode) * newnallocated);
			if (!ptr)
				goto error;
			elems = ptr;
			nallocated = newnallocated;
		}
		elems[nelems] = elem;
		nelems++;
	}

	assert(*curtok);   // TODO: report error "unexpected end of file"
	assert((*curtok)->str.len == 1 && (*curtok)->str.val[0] == ']');
	*curtok = (*curtok)->next;   // skip ]

	// this can't fail because it doesn't actually allocate more, it frees allocated mem
	elems = realloc(elems, sizeof(struct AstNode) * nelems);
	assert(elems);
	nallocated = nelems;

	struct AstArrayOrBlockInfo *info = malloc(sizeof(struct AstArrayOrBlockInfo));
	if (!info)
		goto error;
	info->items = elems;
	info->nitems = nelems;

	struct AstNode *res = new_expression(AST_ARRAY, info);
	if (!res) {
		// must not call free_info(info) because 'goto error;' frees elems
		free(info);
		goto error;
	}
	return res;

error:
	for (size_t i=0; i < nelems; i++)
		astnode_free(elems[i]);
	if (elems)
		free(elems);
	return NULL;
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


struct AstNode *parse_expression(struct Token **curtok)
{
	struct AstNode *res;
	switch ((*curtok)->kind) {
	case TOKEN_STR:
		res = parse_string(curtok);
		break;
	case TOKEN_INT:
		res = parse_int(curtok);
		break;
	case TOKEN_ID:
		res = parse_getvar(curtok);
		break;
	case TOKEN_OP:
		if ((*curtok)->str.len == 1) {
			res = parse_array(curtok);
			break;
		}
	default:
		// TODO: report error "expected this, that or those, got '%s'"
		assert(0);
	}
	if (!res)       // no mem
		return NULL;

	// attributes
	while ((*curtok) && (*curtok)->kind == TOKEN_OP && (*curtok)->str.len == 1 && (*curtok)->str.val[0] == '.') {
		*curtok = (*curtok)->next;   // skip '.'
		assert((*curtok));     // TODO: report error "expected an attribute name, but the file ended"
		assert((*curtok)->kind == TOKEN_ID);   // TODO: report error "invalid attribute name 'bla bla'"

		struct AstGetAttrInfo *gainfo = malloc(sizeof(struct AstGetAttrInfo));
		if(!gainfo) {
			astnode_free(res);
			return NULL;
		}

		gainfo->obj = res;
		if (unicodestring_copyinto((*curtok)->str, &(gainfo->attr)) != STATUS_OK) {
			free(gainfo);
			astnode_free(res);
			return NULL;
		}
		*curtok = (*curtok)->next;

		struct AstNode *ga = new_expression(AST_GETATTR, gainfo);
		if(!ga) {
			free(gainfo->attr.val);
			free(gainfo);
			astnode_free(res);
			return NULL;
		}
		res = ga;
	}

	return res;
}


// because i'm lazy
#define MAX_ARGS 100

// this takes the function as an already-parsed argument
// this way parse_statement() knows when this should be called
static struct AstNode *parse_call(struct Token **curtok, struct AstNode *func)
{
	size_t lineno = (*curtok)->lineno;

	// TODO: when should func be free()'d?
	struct AstCallInfo *callinfo = malloc(sizeof(struct AstCallInfo));
	if (!callinfo)
		return NULL;
	callinfo->func = func;

	callinfo->args = malloc(sizeof(struct AstNode) * MAX_ARGS);
	if (!callinfo->args)
		return NULL;

	for (callinfo->nargs = 0; expression_coming_up(*curtok) && callinfo->nargs < MAX_ARGS; callinfo->nargs++) {
		struct AstNode *arg = parse_expression(curtok);
		if(!arg) {
			for(size_t i=0; i < callinfo->nargs; i++)
				astnode_free(callinfo->args[i]);
			free(callinfo->args);
			free(callinfo);
			return NULL;
		}
		callinfo->args[callinfo->nargs] = arg;
	}

	// TODO: report error "more than MAX_ARGS arguments"
	assert(!expression_coming_up(*curtok));

	// this can't fail because this is freeing memory, not allocating more
	callinfo->args = realloc(callinfo->args, sizeof(struct AstNode) * callinfo->nargs);
	if (callinfo->nargs)       // 0 bytes of memory *MAY* be represented as NULL
		assert(callinfo->args);

	struct AstNode *res = new_statement(AST_CALL, lineno, callinfo);
	if (!res) {
		free_info(AST_CALL, callinfo);     // should free everything allocated here
		return NULL;
	}
	return res;
}
#undef MAX_ARGS

struct AstNode *parse_statement(struct Token **curtok)
{
	// TODO: var statements and assignments
	struct AstNode *func = parse_expression(curtok);
	if (!func)
		return NULL;

	struct AstNode *statement = parse_call(curtok, func);
	assert(!expression_coming_up(*curtok));
	if (!statement) {
		astnode_free(func);
		return NULL;
	}

	// TODO: report error "missing ';'"
	assert((*curtok)->kind == TOKEN_OP);
	assert((*curtok)->str.len == 1);
	assert((*curtok)->str.val[0] == ';');
	*curtok = (*curtok)->next;
	return statement;
}
