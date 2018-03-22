// some assert()s in this file have a comment like 'TODO: report error'
// it means that invalid syntax causes that assert() to fail

#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include "ast.h"
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
			ast_freenode(info_as(AstArrayOrBlockInfo)->items[i]);
		free(info_as(AstArrayOrBlockInfo)->items);
		break;
	case AST_GETVAR:
		free_info(AST_STR, info_as(AstGetVarInfo)->varname);
		break;
	case AST_GETATTR:
		ast_freenode(info_as(AstGetAttrInfo)->obj);
		free_info(AST_STR, info_as(AstGetAttrInfo)->attr);
		break;
	case AST_CREATEVAR:
	case AST_SETVAR:
		free_info(AST_STR, info_as(AstCreateOrSetVarInfo)->varname);
		ast_freenode(info_as(AstCreateOrSetVarInfo)->val);
		break;
	case AST_SETATTR:
		ast_freenode(info_as(AstSetAttrInfo)->obj);
		free_info(AST_STR, info_as(AstSetAttrInfo)->attr);
		ast_freenode(info_as(AstSetAttrInfo)->val);
		break;
	case AST_CALL:
		ast_freenode(info_as(AstCallInfo)->func);
		for (size_t i=0; i < info_as(AstCallInfo)->nargs; i++)
			ast_freenode(info_as(AstCallInfo)->args[i]);
		free(info_as(AstCallInfo)->args);
		break;
#undef info_as
	default:
		assert(0);  // unknown kind
	}
	free(info);
}

void ast_freenode(struct AstNode *node)
{
	free_info(node->kind, node->info);
	free(node);
}


static void *copy_info(char kind, void *info)
{
	// can't use switch because each of these defines a res with a different type
#define info_as(X) ((struct X *) info)
	if(kind == AST_STR) {
		struct AstStrInfo *res = malloc(sizeof(struct AstStrInfo));
		if (!res)
			return NULL;
		res->len = info_as(AstStrInfo)->len;
		res->val = malloc(sizeof(unsigned long) * res->len);
		if(!res->val) {
			free(res);
			return NULL;
		}
		memcpy(res->val, info_as(AstStrInfo)->val,sizeof(unsigned long) * res->len );
		return res;
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
			struct AstNode *item = ast_copynode(info_as(AstArrayOrBlockInfo)->items[i]);
			if(!item) {
				for(size_t j=0; j<i; j++)
					ast_freenode(res->items[j]);
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
		res->varname = copy_info(AST_STR, info_as(AstGetVarInfo)->varname);
		if(!(res->varname)) {
			free(res);
			return NULL;
		}
		return res;
	}
	if (kind == AST_GETATTR) {
		struct AstGetAttrInfo *res = malloc(sizeof(struct AstGetAttrInfo));
		if(!res)
			return NULL;
		res->attr = copy_info(AST_STR, info_as(AstGetAttrInfo)->attr);
		if(!(res->attr)){
			free(res);
			return NULL;
		}
		res->obj = ast_copynode(info_as(AstGetAttrInfo)->obj);
		if(!(res->obj)) {
			free_info(AST_STR, res->attr);
			free(res);
			return NULL;
		}
		return res;
	}
	if (kind == AST_CREATEVAR || kind == AST_SETVAR) {
		struct AstCreateOrSetVarInfo *res = malloc(sizeof(struct AstCreateOrSetVarInfo));
		if (!res)
			return NULL;
		res->varname = copy_info(AST_STR, info_as(AstCreateOrSetVarInfo)->varname);
		if(!res->varname) {
			free(res);
			return NULL;
		}
		res->val = ast_copynode(info_as(AstCreateOrSetVarInfo)->val);
		if (!res->val) {
			free_info(AST_STR, res->varname);
			free(res);
			return NULL;
		}
		return res;
	}
	if (kind == AST_SETATTR) {
		struct AstSetAttrInfo *res = malloc(sizeof(struct AstSetAttrInfo));
		if(!res)
			return NULL;
		res->obj = ast_copynode(info_as(AstSetAttrInfo)->obj);
		if(!res->obj) {
			free(res);
			return NULL;
		}
		res->attr = copy_info(AST_STR, info_as(AstSetAttrInfo)->attr);
		if(!res->attr) {
			ast_freenode(res->obj);
			free(res);
			return NULL;
		}
		res->val = ast_copynode(info_as(AstSetAttrInfo)->val);
		if(!res->val) {
			free_info(AST_STR, res->attr);
			ast_freenode(res->obj);
			free(res);
			return NULL;
		}
		return res;
	}
	if (kind == AST_CALL) {
		struct AstCallInfo *res = malloc(sizeof(struct AstCallInfo));
		if(!res)
			return NULL;
		res->func = ast_copynode(info_as(AstCallInfo)->func);
		if(!(res->func)) {
			ast_freenode(res->func);
			free(res);
			return NULL;
		}
		res->nargs = info_as(AstCallInfo)->nargs;
		res->args = malloc(sizeof(struct AstNode*) * res->nargs);
		if(!res->args){
			ast_freenode(res->func);
			free(res);
			return NULL;
		}
		for (size_t i=0; i < res->nargs; i++) {
			struct AstNode *arg = ast_copynode(info_as(AstCallInfo)->args[i]);
			if(!arg) {
				for(size_t j=0; j<i; j++)
					ast_freenode(res->args[j]);
				free(res->args);
				ast_freenode(res->func);
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

struct AstNode *ast_copynode(struct AstNode *node) {
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
	struct AstStrInfo *res = malloc(sizeof(struct AstStrInfo));
	if (!res)
		return NULL;
	res->len = (*curtok)->vallen;
	res->val = malloc(sizeof(unsigned long) * res->len);
	if (!res->val) {
		free(res);
		return NULL;
	}
	memcpy(res->val, (*curtok)->val, sizeof(unsigned long) * res->len);
	*curtok = (*curtok)->next;
	return res;
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
	info->len = (*curtok)->vallen - 2;
	info->val = malloc(sizeof(unsigned long) * info->len);
	if (!(info->val)) {
		free(info);
		return NULL;
	}
	memcpy(info->val, (*curtok)->val+1, sizeof(unsigned long) * info->len);

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

	info->valstr = malloc((*curtok)->vallen+1);
	if (!(info->valstr)) {
		free(info);
		return NULL;
	}

	// can't use memcpy because curtok->val is unsigned long *
	// TODO: should construct the integer object here??
	for (size_t i=0; i < (*curtok)->vallen; i++)
		info->valstr[i] = (char) ((*curtok)->val[i]);
	info->valstr[(*curtok)->vallen] = 0;

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

	info->varname = strinfo_from_idtoken(curtok);
	if (!info->varname) {
		free(info);
		return NULL;
	}

	struct AstNode *res = new_expression(AST_GETVAR, info);
	if (!res) {
		free_info(AST_STR, info->varname);
		free(info);
		return NULL;
	}
	return res;
}


static struct AstNode *parse_array(struct Token **curtok)
{
	assert(*curtok);
	assert((*curtok)->kind == TOKEN_OP);
	assert((*curtok)->vallen == 1);
	assert((*curtok)->val[0] == (unsigned long) '[');
	*curtok = (*curtok)->next;
	assert(*curtok);    // TODO: report error "unexpected end of file"

	// i thought about looping over the elements twice and then doing just one
	// malloc, but parsing the list items means many mallocs so this is probably
	// faster
	size_t nelems = 0;
	size_t nallocated = 0;
	struct AstNode **elems = NULL;

	while ((*curtok) && !((*curtok)->kind == TOKEN_OP && (*curtok)->vallen == 1 && (*curtok)->val[0] == (unsigned long)']')) {
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
	assert((*curtok)->vallen == 1 && (*curtok)->val[0] == (unsigned long) ']');
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
		ast_freenode(elems[i]);
	if (elems)
		free(elems);
	return NULL;
}


// remember to change this if you add more expressions!
static int expression_coming_up(struct Token *curtok)
{
	if ((!curtok) || (!(curtok->next)))
		return 0;
	if (curtok->next->kind == TOKEN_STR || curtok->next->kind == TOKEN_INT || curtok->next->kind == TOKEN_ID)
		return 1;
#define f(x) (curtok->next->val[0] == (unsigned long)(x))
	if (curtok->next->kind == TOKEN_OP)
		return f('(') || f('{') || f('[');
#undef f
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
		if ((*curtok)->vallen == 1) {
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
	while ((*curtok) && (*curtok)->kind == TOKEN_OP && (*curtok)->vallen == 1 && (*curtok)->val[0] == (unsigned long)'.') {
		*curtok = (*curtok)->next;   // skip '.'
		assert((*curtok));     // TODO: report error "expected an attribute name, but the file ended"
		assert((*curtok)->kind == TOKEN_ID);   // TODO: report error "invalid attribute name 'bla bla'"

		struct AstGetAttrInfo *gainfo = malloc(sizeof(struct AstGetAttrInfo));
		if(!gainfo) {
			ast_freenode(res);
			return NULL;
		}

		gainfo->obj = res;
		gainfo->attr = strinfo_from_idtoken(curtok);
		if (!gainfo->attr) {
			free(gainfo);
			ast_freenode(res);
			return NULL;
		}

		struct AstNode *ga = new_expression(AST_GETATTR, gainfo);
		if(!ga) {
			free_info(AST_STR, gainfo->attr);
			free(gainfo);
			ast_freenode(res);
			return NULL;
		}
		res = ga;
	}

	return res;
}
