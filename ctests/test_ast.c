#include <src/ast.h>
#include <src/tokenizer.h>
#include <src/utf8.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "utils.h"


static struct AstNode *newnode(char kind, void *info)
{
	struct AstNode *res = bmalloc(sizeof(struct AstNode));
	res->kind = kind;
	res->lineno = 123;
	res->info = info;
	return res;
}

static void create_string(struct AstStrInfo **target)
{
	(*target) = bmalloc(sizeof(struct AstStrInfo));
	(*target)->len = 2;
	(*target)->val = bmalloc(sizeof(unicode_t) * 2);
	(*target)->val[0] = (unicode_t) 'x';
	(*target)->val[1] = (unicode_t) 'y';
}

void test_node_structs_and_ast_copynode(void)
{
	struct AstStrInfo *strinfo;
	create_string(&strinfo);
	struct AstNode *strnode = newnode(AST_STR, strinfo);

	struct AstIntInfo *intinfo = bmalloc(sizeof(struct AstIntInfo));
	intinfo->valstr = bmalloc(4);
	strcpy(intinfo->valstr, "123");
	struct AstNode *intnode = newnode(AST_INT, intinfo);

	// freeing the array frees intnode and strnode
	struct AstArrayOrBlockInfo *arrinfo = bmalloc(sizeof(struct AstArrayOrBlockInfo));
	arrinfo->nitems = 2;
	arrinfo->items = bmalloc(sizeof(struct AstNode*) * 2);
	arrinfo->items[0] = strnode;
	arrinfo->items[1] = intnode;
	struct AstNode *arrnode = newnode(AST_ARRAY, arrinfo);

	// freeing this frees arrnode
	struct AstArrayOrBlockInfo *blockinfo = bmalloc(sizeof(struct AstArrayOrBlockInfo));
	blockinfo->items = bmalloc(sizeof(struct AstNode*) * 1);
	blockinfo->items[0] = arrnode;   // lol, not really a statement
	blockinfo->nitems = 1;
	struct AstNode *blocknode = newnode(AST_BLOCK, blockinfo);

	struct AstGetVarInfo *getvarinfo = bmalloc(sizeof(struct AstGetVarInfo));
	create_string(&(getvarinfo->varname));
	struct AstNode *getvarnode = newnode(AST_GETVAR, getvarinfo);

	// freeing this frees getvarnode
	struct AstGetAttrInfo *getattrinfo = bmalloc(sizeof(struct AstGetAttrInfo));
	getattrinfo->obj = getvarnode;
	create_string(&(getattrinfo->attr));
	struct AstNode *getattrnode = newnode(AST_GETATTR, getattrinfo);

	// freeing this frees blocknode
	struct AstCreateOrSetVarInfo *cosvinfo = bmalloc(sizeof(struct AstCreateOrSetVarInfo));
	create_string(&(cosvinfo->varname));
	cosvinfo->val = blocknode;
	// choose AST_CREATEVAR or AST_SETVAR randomly-ish
	struct AstNode *cosvnode = newnode((((int) time(NULL))%2 ? AST_CREATEVAR : AST_SETVAR), cosvinfo);

	// freeing this frees getattrnode and cosvnode
	struct AstSetAttrInfo *setattrinfo = bmalloc(sizeof(struct AstSetAttrInfo));
	setattrinfo->obj = getattrnode;
	create_string(&(setattrinfo->attr));
	setattrinfo->val = cosvnode;
	struct AstNode *setattrnode = newnode(AST_SETATTR, setattrinfo);

	// freeing this frees setattrnode
	struct AstCallInfo *callinfo = bmalloc(sizeof(struct AstCallInfo));
	callinfo->func = setattrnode;
	callinfo->args = bmalloc(123);   // anything free()able will do
	callinfo->nargs = 0;    // it's best to test special cases and corner cases :D
	struct AstNode *callnode = newnode(AST_CALL, callinfo);

	// this is very recursive, should test copying every kind of node that astnode_copy can do
	struct AstNode *callnode2 = astnode_copy(callnode);
	buttert(callnode2);

	// these  should free every object exactly once, check with valgrind
	astnode_free(callnode);
	astnode_free(callnode2);
}


// this assumes that s is ascii for simplicity
static struct AstNode *parsestring(char *s)
{
	unicode_t *hugestring = bmalloc(sizeof(unicode_t) * strlen(s));
	// can't use memcpy because types differ
	for (size_t i=0; i < strlen(s); i++)
		hugestring[i] = (unicode_t) s[i];

	struct Token *tok1st = token_ize(hugestring, strlen(s));
	buttert(tok1st);
	free(hugestring);

	struct Token *tmp = tok1st;
	struct AstNode *node = parse_expression(&tmp);   // changes the address that tmp points to
	token_freeall(tok1st);
	buttert2(node, s);
	return node;
}

static int stringinfo_equals_ascii_charp(struct AstStrInfo *strinfo, char *charp)
{
	if(strinfo->len != strlen(charp))
		return 0;
	for (size_t i=0; i < strinfo->len; i++) {
		if (strinfo->val[i] != (unicode_t) charp[i])
			return 0;
	}
	return 1;
}

void test_strings(void)
{
	struct AstNode *node = parsestring("\"hello\"");
	buttert(node->kind == AST_STR);
	struct AstStrInfo *info = node->info;
	buttert(info->len == 5);
	astnode_free(node);
}

void test_ints(void)
{
	struct AstNode *node = parsestring("-123");
	buttert(node->kind == AST_INT);
	struct AstIntInfo *info = node->info;
	buttert(strcmp(info->valstr, "-123") == 0);
	astnode_free(node);
}

void test_arrays(void)
{
	struct AstNode *node = parsestring("[ \"a\" 123 ]");
	buttert(node->kind == AST_ARRAY);
	struct AstArrayOrBlockInfo *info = node->info;
	buttert(info->nitems == 2);
	buttert(info->items[0]->kind == AST_STR);
	buttert(info->items[1]->kind == AST_INT);
	astnode_free(node);
}

void test_getvars(void)
{
	struct AstNode *node = parsestring("abc");
	buttert(node->kind == AST_GETVAR);
	struct AstGetVarInfo *info = node->info;
	buttert(stringinfo_equals_ascii_charp(info->varname, "abc"));
	astnode_free(node);
}

void test_attributes(void)
{
	struct AstNode *dotc = parsestring("\"asd\".a.b.c");
	buttert(dotc->kind == AST_GETATTR);
	struct AstGetAttrInfo *dotcinfo = dotc->info;
	buttert(stringinfo_equals_ascii_charp(dotcinfo->attr, "c"));

	struct AstNode *dotb = dotcinfo->obj;
	buttert(dotb->kind == AST_GETATTR);
	struct AstGetAttrInfo *dotbinfo = dotb->info;
	buttert(stringinfo_equals_ascii_charp(dotbinfo->attr, "b"));

	struct AstNode *dota = dotbinfo->obj;
	buttert(dota->kind == AST_GETATTR);
	struct AstGetAttrInfo *dotainfo = dota->info;
	buttert(stringinfo_equals_ascii_charp(dotainfo->attr, "a"));

	struct AstNode *str = dotainfo->obj;
	buttert(str->kind == AST_STR);
	struct AstStrInfo *strinfo = str->info;
	buttert(stringinfo_equals_ascii_charp(strinfo, "asd"));

	astnode_free(dotc);
}

TESTS_MAIN(test_node_structs_and_ast_copynode, test_strings, test_ints,
	test_arrays, test_getvars, test_attributes, NULL)
