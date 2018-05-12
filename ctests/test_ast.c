#include <src/ast.h>
#include <src/tokenizer.h>
#include <src/unicode.h>
#include <src/objects/classobject.h>
#include <src/objects/string.h>
#include <src/objects/integer.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "utils.h"


static struct Object *newnode(char kind, void *info)
{
	struct AstNodeData *data = bmalloc(sizeof(struct AstNodeData));
	data->kind = kind;
	data->lineno = 123;
	data->info = info;

	buttert(testinterp->astnodeclass);
	struct Object *node = classobject_newinstance(testinterp, NULL, testinterp->astnodeclass, data);
	buttert(node);
	return node;
}

static void setup_string(struct UnicodeString *target)
{
	target->len = 2;
	target->val = bmalloc(sizeof(unicode_char) * 2);
	target->val[0] = 'x';
	target->val[1] = 'y';
}

// not really random at all, that's why lol
#define RANDOM_CHOICE_LOL(a, b) ( ((int) time(NULL)) % 2 == 0 ? (a) : (b) )

void test_ast_nodes_and_their_refcount_stuff(void)
{
	struct Object *strinfo = stringobject_newfromcharptr(testinterp, NULL, "asd");
	struct Object *strnode = newnode(AST_STR, strinfo);

	struct Object *intinfo = integerobject_newfromcharptr(testinterp, NULL, "123");
	struct Object *intnode = newnode(AST_INT, intinfo);

	// the array references intnode and strnode
	struct AstArrayOrBlockInfo *arrinfo = bmalloc(sizeof(struct AstArrayOrBlockInfo));
	arrinfo->nitems = 2;
	arrinfo->itemnodes = bmalloc(sizeof(struct Object*) * 2);
	arrinfo->itemnodes[0] = strnode;
	arrinfo->itemnodes[1] = intnode;
	struct Object *arrnode = newnode(AST_ARRAY, arrinfo);

	// this references arrnode
	struct AstArrayOrBlockInfo *blockinfo = bmalloc(sizeof(struct AstArrayOrBlockInfo));
	blockinfo->itemnodes = bmalloc(sizeof(struct Object*));
	blockinfo->itemnodes[0] = arrnode;   // lol, not really a statement
	blockinfo->nitems = 1;
	struct Object *blocknode = newnode(AST_BLOCK, blockinfo);

	struct AstGetVarInfo *getvarinfo = bmalloc(sizeof(struct AstGetVarInfo));
	setup_string(&(getvarinfo->varname));
	struct Object *getvarnode = newnode(AST_GETVAR, getvarinfo);

	// this references getvarnode
	struct AstGetAttrOrMethodInfo *gaminfo = bmalloc(sizeof(struct AstGetAttrOrMethodInfo));
	gaminfo->objnode = getvarnode;
	setup_string(&(gaminfo->name));
	struct Object *gamnode = newnode(RANDOM_CHOICE_LOL(AST_GETATTR, AST_GETMETHOD), gaminfo);

	// this references blocknode
	struct AstCreateOrSetVarInfo *cosvinfo = bmalloc(sizeof(struct AstCreateOrSetVarInfo));
	setup_string(&(cosvinfo->varname));
	cosvinfo->valnode = blocknode;
	struct Object *cosvnode = newnode(RANDOM_CHOICE_LOL(AST_CREATEVAR, AST_SETVAR), cosvinfo);

	// this references gamnode and cosvnode
	struct AstSetAttrInfo *setattrinfo = bmalloc(sizeof(struct AstSetAttrInfo));
	setattrinfo->objnode = gamnode;
	setup_string(&(setattrinfo->attr));
	setattrinfo->valnode = cosvnode;
	struct Object *setattrnode = newnode(AST_SETATTR, setattrinfo);

	// this references setattrnode
	struct AstCallInfo *callinfo = bmalloc(sizeof(struct AstCallInfo));
	callinfo->funcnode = setattrnode;
	callinfo->nargs = 0;    // it's best to test special cases and corner cases :D
	callinfo->argnodes = bmalloc(123);   // anything free()able will do
	struct Object *callnode = newnode(AST_CALL, callinfo);

	OBJECT_DECREF(testinterp, callnode);      // should free everything
}


// these assume ascii for simplicity
static struct Object *parse_expression_string(char *s)
{
	struct UnicodeString hugestring;
	hugestring.len = strlen(s);
	hugestring.val = bmalloc(sizeof(unicode_char) * hugestring.len);
	// can't use memcpy because types differ
	for (size_t i=0; i < strlen(s); i++)
		hugestring.val[i] = s[i];

	struct Token *tok1st = token_ize(hugestring);
	buttert(tok1st);
	free(hugestring.val);

	struct Token *tmp = tok1st;
	struct Object *node = ast_parse_expression(testinterp, NULL, &tmp);
	buttert(!tmp);
	token_freeall(tok1st);
	buttert2(node, s);
	return node;
}
static struct Object *parse_statement_string(char *s)
{
	struct UnicodeString hugestring;
	hugestring.len = strlen(s);
	hugestring.val = bmalloc(sizeof(unicode_char) * hugestring.len);
	// can't use memcpy because types differ
	for (size_t i=0; i < hugestring.len; i++)
		hugestring.val[i] = s[i];

	struct Token *tok1st = token_ize(hugestring);
	buttert(tok1st);
	free(hugestring.val);

	struct Token *tmp = tok1st;
	struct Object *node = ast_parse_statement(testinterp, NULL, &tmp);
	token_freeall(tok1st);
	buttert2(node, s);
	return node;
}

// THIS USES ASCII
static int ustr_equals_charp(struct UnicodeString ustr, char *charp)
{
	if(ustr.len != strlen(charp))
		return 0;

	for (size_t i=0; i < ustr.len; i++) {
		if (ustr.val[i] != (unicode_char) charp[i])
			return 0;
	}
	return 1;
}

// THIS USES ASCII
static int stringobject_equals_charp(struct Object *strobj, char *charp)
{
	return ustr_equals_charp(*((struct UnicodeString *) strobj->data), charp);
}


void test_ast_strings(void)
{
	struct Object *node = parse_expression_string("\"hello\"");
	struct AstNodeData *data = node->data;

	buttert(data->kind == AST_STR);
	buttert(stringobject_equals_charp(data->info, "hello"));
	OBJECT_DECREF(testinterp, node);
}

void test_ast_ints(void)
{
	struct Object *node = parse_expression_string("-123");
	struct AstNodeData *data = node->data;

	buttert(data->kind == AST_INT);
	buttert(integerobject_tolonglong(data->info) == -123);
	OBJECT_DECREF(testinterp, node);
}

void test_ast_arrays(void)
{
	struct Object *node = parse_expression_string("[ \"a\" 123 ]");
	struct AstNodeData *data = node->data;
	struct AstArrayOrBlockInfo *info = data->info;

	buttert(data->kind == AST_ARRAY);
	buttert(info->nitems == 2);
	buttert(((struct AstNodeData *) info->itemnodes[0]->data)->kind == AST_STR);
	buttert(((struct AstNodeData *) info->itemnodes[1]->data)->kind == AST_INT);
	OBJECT_DECREF(testinterp, node);
}

void test_ast_getvars(void)
{
	struct Object *node = parse_expression_string("abc");
	struct AstNodeData *data = node->data;
	struct AstGetVarInfo *info = data->info;

	buttert(data->kind == AST_GETVAR);
	buttert(ustr_equals_charp(info->varname, "abc"));
	OBJECT_DECREF(testinterp, node);
}

struct Object *check_attribute_or_method(struct Object *node, char kind, char *name)
{
	struct AstNodeData *data = node->data;
	struct AstGetAttrOrMethodInfo *info = data->info;

	buttert(data->kind == kind);
	buttert(ustr_equals_charp(info->name, name));

	// funny convenience weirdness
	return info->objnode;
}

void test_ast_attributes_and_methods(void)
{
	struct Object *dotc = parse_expression_string("\"asd\".a::b.c");
	struct Object *dotb = check_attribute_or_method(dotc, AST_GETATTR, "c");
	struct Object *dota = check_attribute_or_method(dotb, AST_GETMETHOD, "b");
	struct Object *str = check_attribute_or_method(dota, AST_GETATTR, "a");
	buttert(((struct AstNodeData *) str->data)->kind == AST_STR);

	OBJECT_DECREF(testinterp, dotc);
}

static void check_getvar(struct Object *node, char *varname)
{
	struct AstNodeData *data = node->data;
	buttert(data->kind == AST_GETVAR);
	struct AstGetVarInfo *info = data->info;
	buttert(ustr_equals_charp(info->varname, varname));
}

void test_ast_function_call_statement(void)
{
	struct Object *call = parse_statement_string("a b c;");
	struct AstNodeData *calldata = call->data;
	struct AstCallInfo *callinfo = calldata->info;
	buttert(calldata->kind == AST_CALL);
	buttert(callinfo->nargs == 2);

	check_getvar(callinfo->funcnode, "a");
	check_getvar(callinfo->argnodes[0], "b");
	check_getvar(callinfo->argnodes[1], "c");

	OBJECT_DECREF(testinterp, call);
}
