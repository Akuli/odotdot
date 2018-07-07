#include <src/objectsystem.h>
#include <src/tokenizer.h>
#include <src/unicode.h>
#include <src/objects/array.h>
#include <src/objects/astnode.h>
#include <src/objects/integer.h>
#include <src/objects/mapping.h>
#include <src/operator.h>
#include <src/objects/string.h>
#include <src/parse.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "utils.h"


static struct Object *newnode(char kind, void *info)
{
	struct Object *res = astnodeobject_new(testinterp, kind, "<test>", 123, info);
	buttert(res);
	return res;
}

// not really random at all, that's why lol
#define RANDOM_CHOICE_LOL(a, b) ( ((int) time(NULL)) % 2 == 0 ? (a) : (b) )

static unicode_char onetwothreeval[] = { '1', '2', '3' };
static struct UnicodeString onetwothree = { .len = 3, .val = onetwothreeval };

void test_ast_nodes_and_their_refcount_stuff(void)
{
	struct Object *strinfo = stringobject_newfromcharptr(testinterp, "asd");
	struct Object *strnode = newnode(AST_STR, strinfo);

	struct Object *intinfo = integerobject_newfromustr(testinterp, onetwothree);
	struct Object *intnode = newnode(AST_INT, intinfo);

	// the array references intnode and strnode
	struct AstArrayOrBlockInfo *arrinfo = arrayobject_newempty(testinterp);
	buttert(arrinfo);
	buttert(arrayobject_push(testinterp, arrinfo, strnode) == true);
	buttert(arrayobject_push(testinterp, arrinfo, intnode) == true);
	OBJECT_DECREF(testinterp, strnode);
	OBJECT_DECREF(testinterp, intnode);
	struct Object *arrnode = newnode(RANDOM_CHOICE_LOL(AST_ARRAY, AST_BLOCK), arrinfo);

	struct AstGetVarInfo *getvarinfo = bmalloc(sizeof(struct AstGetVarInfo));
	buttert((getvarinfo->varname = stringobject_newfromcharptr(testinterp, "toottoot")));
	struct Object *getvarnode = newnode(AST_GETVAR, getvarinfo);

	// this references getvarnode
	struct AstGetAttrInfo *getattrinfo = bmalloc(sizeof(struct AstGetAttrInfo));
	getattrinfo->objnode = getvarnode;
	buttert((getattrinfo->name = stringobject_newfromcharptr(testinterp, "wwut")));
	struct Object *getattrnode = newnode(AST_GETATTR, getattrinfo);

	// this references arrnode
	struct AstCreateOrSetVarInfo *cosvinfo = bmalloc(sizeof(struct AstCreateOrSetVarInfo));
	buttert((cosvinfo->varname = stringobject_newfromcharptr(testinterp, "wow")));
	cosvinfo->valnode = arrnode;
	struct Object *cosvnode = newnode(RANDOM_CHOICE_LOL(AST_CREATEVAR, AST_SETVAR), cosvinfo);

	// this references getattrnode and cosvnode
	struct AstSetAttrInfo *setattrinfo = bmalloc(sizeof(struct AstSetAttrInfo));
	setattrinfo->objnode = getattrnode;
	buttert((setattrinfo->attr = stringobject_newfromcharptr(testinterp, "lol")));
	setattrinfo->valnode = cosvnode;
	struct Object *setattrnode = newnode(AST_SETATTR, setattrinfo);

	// this references setattrnode
	struct AstCallInfo *callinfo = bmalloc(sizeof(struct AstCallInfo));
	callinfo->funcnode = setattrnode;
	buttert((callinfo->args = arrayobject_newempty(testinterp)));
	buttert((callinfo->opts = mappingobject_newempty(testinterp)));
	struct Object *callnode = newnode(AST_CALL, callinfo);
	buttert(callnode);

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

	struct Token *tok1st = token_ize(testinterp, hugestring);
	buttert(tok1st);
	free(hugestring.val);

	struct Token *tmp = tok1st;
	struct Object *node = parse_expression(testinterp, "<test>", &tmp);
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

	struct Token *tok1st = token_ize(testinterp, hugestring);
	buttert(tok1st);
	free(hugestring.val);

	struct Token *tmp = tok1st;
	struct Object *node = parse_statement(testinterp, "<test>", &tmp);
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
	return ustr_equals_charp(*((struct UnicodeString *) strobj->objdata.data), charp);
}


void test_ast_strings(void)
{
	struct Object *node = parse_expression_string("\"hello\"");
	struct AstNodeObjectData *data = node->objdata.data;

	buttert(data->kind == AST_STR);
	buttert(stringobject_equals_charp(data->info, "hello"));
	OBJECT_DECREF(testinterp, node);
}

void test_ast_ints(void)
{
	struct Object *node = parse_expression_string("123");
	struct AstNodeObjectData *data = node->objdata.data;

	buttert(data->kind == AST_INT);
	buttert(integerobject_tolonglong(data->info) == 123);
	OBJECT_DECREF(testinterp, node);
}

void test_ast_arrays(void)
{
	struct Object *node = parse_expression_string("[ \"a\" 123 ]");
	struct AstNodeObjectData *data = node->objdata.data;
	struct AstArrayOrBlockInfo *info = data->info;

	buttert(data->kind == AST_ARRAY);
	buttert(ARRAYOBJECT_LEN(info) == 2);
	buttert(((struct AstNodeObjectData *) ARRAYOBJECT_GET(info, 0)->objdata.data)->kind == AST_STR);
	buttert(((struct AstNodeObjectData *) ARRAYOBJECT_GET(info, 1)->objdata.data)->kind == AST_INT);
	OBJECT_DECREF(testinterp, node);
}

void test_ast_getvars(void)
{
	struct Object *node = parse_expression_string("abc");
	struct AstNodeObjectData *data = node->objdata.data;
	struct AstGetVarInfo *info = data->info;

	buttert(data->kind == AST_GETVAR);

	struct Object *abc = stringobject_newfromcharptr(testinterp, "abc");
	buttert(abc);
	buttert(operator_eqint(testinterp, info->varname, abc) == 1);
	OBJECT_DECREF(testinterp, abc);

	OBJECT_DECREF(testinterp, node);
}

struct Object *check_attribute(struct Object *node, char *name)
{
	struct AstNodeObjectData *data = node->objdata.data;
	struct AstGetAttrInfo *info = data->info;

	buttert(data->kind == AST_GETATTR);
	struct Object *s = stringobject_newfromcharptr(testinterp, name);
	buttert(operator_eqint(testinterp, info->name, s) == 1);
	OBJECT_DECREF(testinterp, s);

	// funny convenience weirdness
	return info->objnode;
}

void test_ast_attributes_and_methods(void)
{
	struct Object *dotc = parse_expression_string("\"asd\".a.b.c");
	struct Object *dotb = check_attribute(dotc, "c");
	struct Object *dota = check_attribute(dotb, "b");
	struct Object *str = check_attribute(dota, "a");
	buttert(((struct AstNodeObjectData *) str->objdata.data)->kind == AST_STR);

	OBJECT_DECREF(testinterp, dotc);
}

static void check_getvar(struct Object *node, char *varname)
{
	struct AstNodeObjectData *data = node->objdata.data;
	buttert(data->kind == AST_GETVAR);
	struct AstGetVarInfo *info = data->info;

	struct Object *s = stringobject_newfromcharptr(testinterp, varname);
	buttert(operator_eqint(testinterp, info->varname, s) == 1);
	OBJECT_DECREF(testinterp, s);
}

void test_ast_function_call_statement(void)
{
	struct Object *call = parse_statement_string("a b c;");
	struct AstNodeObjectData *calldata = call->objdata.data;
	struct AstCallInfo *callinfo = calldata->info;
	buttert(calldata->kind == AST_CALL);
	buttert(ARRAYOBJECT_LEN(callinfo->args) == 2);

	check_getvar(callinfo->funcnode, "a");
	check_getvar(ARRAYOBJECT_GET(callinfo->args, 0), "b");
	check_getvar(ARRAYOBJECT_GET(callinfo->args, 1), "c");

	OBJECT_DECREF(testinterp, call);
}
