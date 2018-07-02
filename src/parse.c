// some assert()s in this file have a comment like 'TODO: report error'
// it means that invalid syntax causes that assert() to fail

// FIXME: nomemerr handling

#include "parse.h"
#include <assert.h>
#include <stdbool.h>
#include <stdlib.h>
#include "interpreter.h"
#include "objectsystem.h"
#include "tokenizer.h"
#include "unicode.h"
#include "objects/array.h"
#include "objects/astnode.h"
#include "objects/classobject.h"
#include "objects/errors.h"
#include "objects/integer.h"
#include "objects/mapping.h"
#include "objects/string.h"

// remember to change this if you add more expressions!
// this never fails
static bool expression_coming_up(struct Token *curtok)
{
	if (!curtok)
		return false;
	if (curtok->kind == TOKEN_STR || curtok->kind == TOKEN_INT || curtok->kind == TOKEN_ID)
		return true;
	if (curtok->kind == TOKEN_OP)
		return curtok->str.len == 1 && (
			curtok->str.val[0] == '(' ||
			curtok->str.val[0] == '{' ||
			curtok->str.val[0] == '[');
	return false;
}


// all parse_blahblah() functions RETURN A NEW REFERENCE or NULL on error


// "asd"
static struct Object *parse_string(struct Interpreter *interp, char *filename, struct Token **curtok)
{
	// these should be checked by the caller
	assert(*curtok);
	assert((*curtok)->kind == TOKEN_STR);

	// tokenizer.c makes sure that the escapes are valid, so there's no need to keep track of the source length
	unicode_char *src = (*curtok)->str.val;

	// the new string's length is never more than (*curtok)->str.len - 2
	// because (*curtok)->str.len includes a " in both sides
	// and 2-character escape sequences represent 1 character in the string
	// e.g. \n (2 characters) represents a newline (1 character)
	// so length of a \n representation is bigger than length of a string with newlines
	unicode_char *dst = malloc(sizeof(unicode_char) * ((*curtok)->str.len - 2));
	if (!dst) {
		errorobject_thrownomem(interp);
		return NULL;
	}
	size_t dstlen = 0;

	// skip initial "
	src++;

	while (*src != '"') {
		if (*src == '\\') {
			src++;
			if (*src == 'n')
				dst[dstlen++] = '\n';
			else if (*src == 't')
				dst[dstlen++] = '\t';
			else if (*src == '\\')
				dst[dstlen++] = '\\';
			else if (*src == '"')
				dst[dstlen++] = '"';
			else
				assert(0);
			src++;
		} else {
			dst[dstlen++] = *src++;
		}
	}

	struct Object *info = stringobject_newfromustr(interp, (struct UnicodeString) { .len = dstlen, .val = dst });
	if (!info)
		return NULL;

	struct Object *res = astnodeobject_new(interp, AST_STR, filename, (*curtok)->lineno, info);
	if (!res) {
		OBJECT_DECREF(interp, info);
		return NULL;
	}

	*curtok = (*curtok)->next;
	return res;
}


// 123, -456
static struct Object *parse_int(struct Interpreter *interp, char *filename, struct Token **curtok)
{
	assert(*curtok);
	assert((*curtok)->kind == TOKEN_INT);

	struct Object *info = integerobject_newfromustr(interp, (*curtok)->str);
	if (!info)
		return NULL;

	struct Object *res = astnodeobject_new(interp, AST_INT, filename, (*curtok)->lineno, info);
	if(!res) {
		OBJECT_DECREF(interp, info);
		return NULL;
	}

	*curtok = (*curtok)->next;
	return res;
}


// x
static struct Object *parse_getvar(struct Interpreter *interp, char *filename, struct Token **curtok)
{
	assert(*curtok);
	assert((*curtok)->kind == TOKEN_ID);

	struct AstGetVarInfo *info = malloc(sizeof(struct AstGetVarInfo));
	if (!info)
		return NULL;

	if (!(info->varname = stringobject_newfromustr_copy(interp, (*curtok)->str))) {
		free(info);
		return NULL;
	}

	struct Object *res = astnodeobject_new(interp, AST_GETVAR, filename, (*curtok)->lineno, info);
	if (!res) {
		OBJECT_DECREF(interp, info->varname);
		free(info);
		return NULL;
	}

	*curtok = (*curtok)->next;
	return res;
}


// func arg1 arg2 opt1:val1 opt2:val2;
// this takes the function as an already-parsed argument
// this way parse_statement() knows when this should be called
static struct Object *parse_call(struct Interpreter *interp, char *filename, struct Token **curtok, struct Object *funcnode)
{
	size_t lineno = (*curtok)->lineno;

	struct AstCallInfo *callinfo = malloc(sizeof(struct AstCallInfo));
	if (!callinfo) {
		errorobject_thrownomem(interp);
		return NULL;
	}

	if (!(callinfo->args = arrayobject_newempty(interp))) {
		free(callinfo);
		return NULL;
	}
	if (!(callinfo->opts = mappingobject_newempty(interp))) {
		OBJECT_DECREF(interp, callinfo->args);
		free(callinfo);
		return NULL;
	}

	callinfo->funcnode = funcnode;
	OBJECT_INCREF(interp, funcnode);

	while (expression_coming_up(*curtok)) {
		if ((*curtok)->kind == TOKEN_ID && (*curtok)->next &&
			(*curtok)->next->kind == TOKEN_OP && (*curtok)->next->str.len == 1 && (*curtok)->next->str.val[0] == ':')
		{
			// opt:val
			struct Object *optstr = stringobject_newfromustr_copy(interp, (*curtok)->str);
			if (!optstr)
				goto error;
			*curtok = (*curtok)->next->next;    // skip opt and :

			struct Object *valnode = parse_expression(interp, filename, curtok);
			if (!valnode) {
				OBJECT_DECREF(interp, optstr);
				goto error;
			}

			bool ok = mappingobject_set(interp, callinfo->opts, optstr, valnode);
			OBJECT_DECREF(interp, optstr);
			OBJECT_DECREF(interp, valnode);
			if (!ok)
				goto error;

		} else {
			struct Object *arg = parse_expression(interp, filename, curtok);
			if(!arg)
				goto error;

			bool ok = arrayobject_push(interp, callinfo->args, arg);
			OBJECT_DECREF(interp, arg);
			if (!ok)
				goto error;
		}
	}

	struct Object *res = astnodeobject_new(interp, AST_CALL, filename, lineno, callinfo);
	if (!res) {
		OBJECT_DECREF(interp, funcnode);
		OBJECT_DECREF(interp, callinfo->args);
		OBJECT_DECREF(interp, callinfo->opts);
		free(callinfo);
		return NULL;
	}
	return res;

error:
	OBJECT_DECREF(interp, funcnode);
	OBJECT_DECREF(interp, callinfo->args);
	OBJECT_DECREF(interp, callinfo->opts);
	free(callinfo);
	return NULL;
}

// arg1 OPERATOR arg2
// where OPERATOR is one of: + - * / == !=
static struct Object *parse_operator_call(struct Interpreter *interp, char *filename, struct Token **curtok, struct Object *lhs)
{
	size_t lineno = (*curtok)->lineno;

	assert((*curtok)->kind == TOKEN_OP);
	struct UnicodeString op = (*curtok)->str;
	*curtok = (*curtok)->next;

	struct Object *rhs = parse_expression(interp, filename, curtok);
	if (!rhs)
		return NULL;

	struct AstOpCallInfo *opcallinfo = malloc(sizeof(struct AstOpCallInfo));
	if (!opcallinfo) {
		OBJECT_DECREF(interp, rhs);
		return NULL;
	}

	if (op.len == 1) {
		if (op.val[0] == '+')
			opcallinfo->op = OPERATOR_ADD;
		else if (op.val[0] == '-')
			opcallinfo->op = OPERATOR_SUB;
		else if (op.val[0] == '*')
			opcallinfo->op = OPERATOR_MUL;
		else if (op.val[0] == '/')
			opcallinfo->op = OPERATOR_DIV;
		else if (op.val[0] == '>')
			opcallinfo->op = OPERATOR_GT;
		else if (op.val[0] == '<')
			opcallinfo->op = OPERATOR_LT;
		else
			assert(0);
	} else if (op.len == 2) {
		if (op.val[0] == '=' && op.val[1] == '=')
			opcallinfo->op = OPERATOR_EQ;
		else if (op.val[0] == '!' && op.val[1] == '=')
			opcallinfo->op = OPERATOR_NE;
		else if (op.val[0] == '>' && op.val[1] == '=')
			opcallinfo->op = OPERATOR_GE;
		else if (op.val[0] == '<' && op.val[1] == '=')
			opcallinfo->op = OPERATOR_LE;
		else
			assert(0);
	} else
		assert(0);

	opcallinfo->lhs = lhs;
	OBJECT_INCREF(interp, lhs);
	opcallinfo->rhs = rhs;
	// already holding a reference to rhs

	struct Object *res = astnodeobject_new(interp, AST_OPCALL, filename, lineno, opcallinfo);
	if (!res) {
		OBJECT_DECREF(interp, opcallinfo->lhs);
		OBJECT_DECREF(interp, opcallinfo->rhs);
		free(opcallinfo);
		return NULL;
	}
	return res;
}


// arg1 `func` arg2
static struct Object *parse_infix_call(struct Interpreter *interp, char *filename, struct Token **curtok, struct Object *arg1)
{
	size_t lineno = (*curtok)->lineno;

	// this SHOULD be checked by parse_expression()
	assert((*curtok)->str.len == 1 && (*curtok)->str.val[0] == '`');
	*curtok = (*curtok)->next;

	struct Object *func = parse_expression(interp, filename, curtok);
	if (!func)
		return NULL;

	if ((*curtok)->str.len != 1 || (*curtok)->str.val[0] != '`') {
		// TODO: report error "expected another `"
		assert(0);
	}
	*curtok = (*curtok)->next;

	struct Object *arg2 = parse_expression(interp, filename, curtok);
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

	// infix calls don't support options, so this is left empty
	if (!(callinfo->opts = mappingobject_newempty(interp))) {
		free(callinfo);
		OBJECT_DECREF(interp, arg2);
		OBJECT_DECREF(interp, func);
		return NULL;
	}

	callinfo->funcnode = func;

	callinfo->args = arrayobject_new(interp, (struct Object *[]) { arg1, arg2 }, 2);
	OBJECT_DECREF(interp, arg2);
	if (!callinfo->args) {
		OBJECT_DECREF(interp, callinfo->opts);
		free(callinfo);
		OBJECT_DECREF(interp, func);
		return NULL;
	}

	struct Object *res = astnodeobject_new(interp, AST_CALL, filename, lineno, callinfo);
	if (!res) {
		OBJECT_DECREF(interp, callinfo->args);
		OBJECT_DECREF(interp, callinfo->opts);
		free(callinfo);
		OBJECT_DECREF(interp, func);
		return NULL;
	}
	return res;
}


//     (func arg1 arg2)
// or: (arg1 `func` arg2)
// or: (arg1 OPERATOR arg2)  where OPERATOR is one of:  + - * / < >
static struct Object *parse_call_expression(struct Interpreter *interp, char *filename, struct Token **curtok)
{
	// this SHOULD be checked by parse_expression()
	assert((*curtok)->str.len == 1 && (*curtok)->str.val[0] == '(');
	*curtok = (*curtok)->next;

	struct Object *first = parse_expression(interp, filename, curtok);
	if (!first)
		return NULL;

	struct Object *res;
#define f(x) ((*curtok)->kind == TOKEN_OP && (*curtok)->str.len == 1 && (*curtok)->str.val[0] == (x))
	if (f('`'))
		res = parse_infix_call(interp, filename, curtok, first);
#define g(x,y) ((*curtok)->kind == TOKEN_OP && (*curtok)->str.len == 2 && (*curtok)->str.val[0] == (x) && (*curtok)->str.val[1] == (y))
	else if (f('+')||f('-')||f('*')||f('/')||f('<')||f('>')||g('!','=')||g('=','=')||g('>','=')||g('<','='))
#undef f
#undef g
		res = parse_operator_call(interp, filename, curtok, first);
	else
		res = parse_call(interp, filename, curtok, first);
	OBJECT_DECREF(interp, first);
	if (!res)
		return NULL;

	// TODO: report error "missing ')'"
	assert((*curtok)->str.len == 1 && (*curtok)->str.val[0] == ')');
	*curtok = (*curtok)->next;

	return res;
}


// [a b c]
static struct Object *parse_array(struct Interpreter *interp, char *filename, struct Token **curtok)
{
	assert(*curtok);
	assert((*curtok)->kind == TOKEN_OP);
	assert((*curtok)->str.len == 1);
	assert((*curtok)->str.val[0] == '[');
	size_t lineno = (*curtok)->lineno;
	*curtok = (*curtok)->next;
	assert(*curtok);    // TODO: report error "unexpected end of file"

	struct Object *elements = arrayobject_newempty(interp);
	if (!elements)
		return NULL;

	while ((*curtok) && !((*curtok)->kind == TOKEN_OP && (*curtok)->str.len == 1 && (*curtok)->str.val[0] == ']')) {
		struct Object *elem = parse_expression(interp, filename, curtok);
		if(!elem) {
			OBJECT_DECREF(interp, elements);
			return NULL;
		}
		bool ok = arrayobject_push(interp, elements, elem );
		OBJECT_DECREF(interp, elem);
		if (!ok) {
			OBJECT_DECREF(interp, elements);
			return NULL;
		}
	}

	assert(*curtok);   // TODO: report error "unexpected end of file"
	assert((*curtok)->str.len == 1 && (*curtok)->str.val[0] == ']');
	*curtok = (*curtok)->next;   // skip ']'

	struct Object *res = astnodeobject_new(interp, AST_ARRAY, filename, lineno, elements);
	if (!res) {
		OBJECT_DECREF(interp, elements);
		return NULL;
	}
	return res;
}

// creates an AstNode that represents getting a variable named return
static struct Object *create_return_getvar(struct Interpreter *interp, char *filename, size_t lineno)
{
	struct AstGetVarInfo *info = malloc(sizeof (struct AstGetVarInfo));
	if (!info) {
		errorobject_thrownomem(interp);
		return NULL;
	}

	info->varname = interp->strings.return_;
	OBJECT_INCREF(interp, info->varname);

	struct Object *res = astnodeobject_new(interp, AST_GETVAR, filename, lineno, info);
	if (!res) {
		OBJECT_DECREF(interp, info->varname);
		free(info);
		return NULL;
	}
	return res;
}

// create an AstNode that represents 'return returnednode;'
static struct Object *create_return_call(struct Interpreter *interp, struct Object *returnednode)
{
	struct AstCallInfo *callinfo = malloc(sizeof(struct AstCallInfo));
	if (!callinfo) {
		errorobject_thrownomem(interp);
		return NULL;
	}

	if (!(callinfo->opts = mappingobject_newempty(interp))) {
		errorobject_thrownomem(interp);
		free(callinfo);
		return NULL;
	}

	struct AstNodeObjectData* tmp = returnednode->objdata.data;
	if (!(callinfo->funcnode = create_return_getvar(interp, tmp->filename, tmp->lineno))) {
		OBJECT_DECREF(interp, callinfo->opts);
		free(callinfo);
		return NULL;
	}

	if (!(callinfo->args = arrayobject_new(interp, &returnednode, 1))) {
		OBJECT_DECREF(interp, callinfo->funcnode);
		OBJECT_DECREF(interp, callinfo->opts);
		free(callinfo);
		return NULL;
	}

	struct Object *res = astnodeobject_new(interp, AST_CALL, tmp->filename, tmp->lineno, callinfo);
	if (!res) {
		OBJECT_DECREF(interp, callinfo->args);
		OBJECT_DECREF(interp, callinfo->funcnode);
		OBJECT_DECREF(interp, callinfo->opts);
		free(callinfo);
		return NULL;
	}
	return res;
}

// { ... }
static struct Object *parse_block(struct Interpreter *interp, char *filename, struct Token **curtok)
{
	assert(*curtok);
	assert((*curtok)->kind == TOKEN_OP);
	assert((*curtok)->str.len == 1);
	assert((*curtok)->str.val[0] == '{');
	size_t lineno = (*curtok)->lineno;
	*curtok = (*curtok)->next;
	assert(*curtok);    // TODO: report error "unexpected end of file"

	// figure out whether it contains a semicolon before }
	// if not, this is an implicit return: { expr } is same as { return expr; }
	bool implicitreturn;

	if ((*curtok)->kind == TOKEN_OP && (*curtok)->str.len == 1 && (*curtok)->str.val[0] == '}') {
		// { } is an empty block
		implicitreturn = false;
	} else {
		// check if there's a ; before }
		// must match braces because blocks can be nested
		unsigned int bracecount = 1;
		bool foundit = false;

		for (struct Token *futtok = *curtok; futtok; futtok = futtok->next) {
			if (futtok->kind != TOKEN_OP || futtok->str.len != 1)
				continue;
			if (futtok->str.val[0] == ';' && bracecount == 1) {
				// found a ; not nested in another { }
				implicitreturn = false;
				foundit = true;
				break;
			}
			if (futtok->str.val[0] == '}' && --bracecount == 0) {
				// found the terminating } before a ;
				implicitreturn = true;
				foundit = true;
				break;
			}
			if (futtok->str.val[0] == '{')
				bracecount++;
		}

		// TODO: report error "missing }"
		assert(foundit);
	}

	struct Object *statements = arrayobject_newempty(interp);
	if (!statements)
		return NULL;

	if (implicitreturn) {
		struct Object *returnme = parse_expression(interp, filename, curtok);
		if (!returnme) {
			OBJECT_DECREF(interp, statements);
			return NULL;
		}

		struct Object *returncall = create_return_call(interp, returnme);
		OBJECT_DECREF(interp, returnme);
		if (!returncall) {
			OBJECT_DECREF(interp, statements);
			return NULL;
		}

		bool ok = arrayobject_push(interp, statements, returncall);
		OBJECT_DECREF(interp, returncall);
		if (!ok) {
			OBJECT_DECREF(interp, statements);
			return NULL;
		}
	} else {
		while ((*curtok) && !((*curtok)->kind == TOKEN_OP && (*curtok)->str.len == 1 && (*curtok)->str.val[0] == '}')) {
			struct Object *stmt = parse_statement(interp, filename, curtok);
			if (!stmt) {
				OBJECT_DECREF(interp, statements);
				return NULL;
			}
			bool ok = arrayobject_push(interp, statements, stmt);
			OBJECT_DECREF(interp, stmt);
			if (!ok) {
				OBJECT_DECREF(interp, statements);
				return NULL;
			}
		}
	};

	assert(*curtok);   // TODO: report error "unexpected end of file"
	assert((*curtok)->str.len == 1 && (*curtok)->str.val[0] == '}');
	*curtok = (*curtok)->next;   // skip ']'

	struct Object *res = astnodeobject_new(interp, AST_BLOCK, filename, lineno, statements);
	if (!res) {
		OBJECT_DECREF(interp, statements);
		return NULL;
	}
	return res;
}


struct Object *parse_expression(struct Interpreter *interp, char *filename, struct Token **curtok)
{
	struct Object *res;
	switch ((*curtok)->kind) {
	case TOKEN_STR:
		res = parse_string(interp, filename, curtok);
		break;
	case TOKEN_INT:
		res = parse_int(interp, filename, curtok);
		break;
	case TOKEN_ID:
		res = parse_getvar(interp, filename, curtok);
		break;
	case TOKEN_OP:
		if ((*curtok)->str.len == 1) {
			if ((*curtok)->str.val[0] == '[') {
				res = parse_array(interp, filename, curtok);
				break;
			}
			if ((*curtok)->str.val[0] == '{') {
				res = parse_block(interp, filename, curtok);
				break;
			}
			if ((*curtok)->str.val[0] == '(') {
				res = parse_call_expression(interp, filename, curtok);
				break;
			}
		}
	default:
		// TODO: report error "expected this, that or those, got '%s'"
		assert(0);
	}
	if (!res)
		return NULL;

	// attributes
	while ((*curtok) && (*curtok)->kind == TOKEN_OP && (*curtok)->str.len == 1 && (*curtok)->str.val[0] == '.') {
		size_t lineno = (*curtok)->lineno;
		*curtok = (*curtok)->next;   // skip '.'
		assert((*curtok));     // TODO: report error "expected an attribute name, but the file ended"
		assert((*curtok)->kind == TOKEN_ID);   // TODO: report error "invalid attribute name 'bla bla'"

		struct AstGetAttrInfo *getattrinfo = malloc(sizeof(struct AstGetAttrInfo));
		if(!getattrinfo) {
			// TODO: set no mem error
			OBJECT_DECREF(interp, res);
			return NULL;
		}

		// no need to incref, this function is already holding a reference to res
		getattrinfo->objnode = res;

		if (!(getattrinfo->name = stringobject_newfromustr_copy(interp, (*curtok)->str))) {
			free(getattrinfo);
			OBJECT_DECREF(interp, res);
			return NULL;
		}
		*curtok = (*curtok)->next;

		struct Object *getattr = astnodeobject_new(interp, AST_GETATTR, filename, lineno, getattrinfo);
		if(!getattr) {
			OBJECT_DECREF(interp, getattrinfo->name);
			free(getattrinfo);
			OBJECT_DECREF(interp, res);
			return NULL;
		}
		res = getattr;
	}

	return res;
}


// var x = y;
static struct Object *parse_var_statement(struct Interpreter *interp, char *filename, struct Token **curtok)
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

	struct Object *varname = stringobject_newfromustr_copy(interp, (*curtok)->str);
	if (!varname)
		return NULL;
	*curtok = (*curtok)->next;

	// TODO: should 'var x;' set x to null? or just be forbidden?
	assert((*curtok)->kind == TOKEN_OP);
	assert((*curtok)->str.len == 1);
	assert((*curtok)->str.val[0] == '=');
	*curtok = (*curtok)->next;

	struct Object *value = parse_expression(interp, filename, curtok);
	if (!value) {
		OBJECT_DECREF(interp, varname);
		return NULL;
	}

	struct AstCreateOrSetVarInfo *info = malloc(sizeof(struct AstCreateOrSetVarInfo));
	if (!info) {
		// TODO: set no mem error
		OBJECT_DECREF(interp, value);
		OBJECT_DECREF(interp, varname);
		return NULL;
	}
	info->varname = varname;
	info->valnode = value;

	struct Object *res = astnodeobject_new(interp, AST_CREATEVAR, filename, lineno, info);
	if (!res) {
		// TODO: set no mem error
		free(info);
		OBJECT_DECREF(interp, value);
		OBJECT_DECREF(interp, varname);
		return NULL;
	}
	return res;
}

// x = y;
static struct Object *parse_assignment(struct Interpreter *interp, char *filename, struct Token **curtok, struct Object *lhs)
{
	struct AstNodeObjectData *lhsdata = lhs->objdata.data;
	if (lhsdata->kind != AST_GETVAR && lhsdata->kind != AST_GETATTR) {
		// TODO: report an error e.g. like this:
		//   the x of 'x = y;' must be a variable name or an attribute
		assert(0);
	}

	// these should be checked by the caller
	assert((*curtok)->str.len == 1);
	assert((*curtok)->str.val[0] == '=');
	*curtok = (*curtok)->next;

	struct Object *rhs = parse_expression(interp, filename, curtok);
	if (!rhs)
		return NULL;

	if (lhsdata->kind == AST_GETVAR) {
		struct AstGetVarInfo *lhsinfo = lhsdata->info;
		struct AstCreateOrSetVarInfo *info = malloc(sizeof(struct AstCreateOrSetVarInfo));
		if (!info)
			goto error;

		info->varname = lhsinfo->varname;
		OBJECT_INCREF(interp, info->varname);
		info->valnode = rhs;

		struct Object *result = astnodeobject_new(interp, AST_SETVAR, filename, lhsdata->lineno, info);
		if (!result) {
			OBJECT_DECREF(interp, info->varname);
			free(info);
			goto error;
		}
		return result;
	} else {
		assert(lhsdata->kind == AST_GETATTR);
		struct AstGetAttrInfo *lhsinfo = lhsdata->info;
		struct AstSetAttrInfo *info = malloc(sizeof(struct AstSetAttrInfo));
		if (!info)
			goto error;

		info->attr = lhsinfo->name;
		OBJECT_INCREF(interp, lhsinfo->name);
		info->objnode = lhsinfo->objnode;
		OBJECT_INCREF(interp, lhsinfo->objnode);
		info->valnode = rhs;

		struct Object *result = astnodeobject_new(interp, AST_SETATTR, filename, lhsdata->lineno, info);
		if (!result) {
			OBJECT_DECREF(interp, lhsinfo->name);
			OBJECT_DECREF(interp, lhsinfo->objnode);
			free(info);
			goto error;
		}
		return result;
	}

error:
	OBJECT_DECREF(interp, rhs);
	return NULL;
}

struct Object *parse_statement(struct Interpreter *interp, char *filename, struct Token **curtok)
{
	struct Object *res;
	if ((*curtok)->kind == TOKEN_KEYWORD) {
		// var is currently the only keyword
		res = parse_var_statement(interp, filename, curtok);
	} else {
		// a function call or an assignment
		struct Object *first = parse_expression(interp, filename, curtok);
		if (!first)
			return NULL;

		if ((*curtok)->kind == TOKEN_OP && (*curtok)->str.len == 1 && (*curtok)->str.val[0] == '=')
			res = parse_assignment(interp, filename, curtok, first);
		// TODO: are infix call statements actually useful for something?
		else if ((*curtok)->kind == TOKEN_OP && (*curtok)->str.len == 1 && (*curtok)->str.val[0] == '`')
			res = parse_infix_call(interp, filename, curtok, first);
		else
			res = parse_call(interp, filename, curtok, first);
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
