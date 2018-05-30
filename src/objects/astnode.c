#include "astnode.h"
#include <assert.h>
#include <stdlib.h>
#include "../common.h"
#include "../interpreter.h"
#include "../objectsystem.h"
#include "../tokenizer.h"
#include "../unicode.h"
#include "array.h"
#include "classobject.h"
#include "errors.h"
#include "integer.h"
#include "string.h"

static void foreachref(struct Object *node, void *cbdata, classobject_foreachrefcb cb)
{
	struct AstNodeObjectData *data = node->data;
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

static void destructor(struct Object *node)
{
	struct AstNodeObjectData *data = node->data;
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

struct Object *astnodeobject_createclass(struct Interpreter *interp)
{
	// the 1 means that AstNode instances may have attributes
	// TODO: add at least kind and lineno attributes to the nodes?
	return classobject_new(interp, "AstNode", interp->builtins.objectclass, 1, foreachref);
}

struct Object *astnodeobject_new(struct Interpreter *interp, char kind, size_t lineno, void *info)
{
	struct AstNodeObjectData *data = malloc(sizeof(struct AstNodeObjectData));
	if (!data) {
		errorobject_setnomem(interp);
		return NULL;
	}
	data->kind = kind;
	data->lineno = lineno;
	data->info = info;

	struct Object *obj = classobject_newinstance(interp, interp->builtins.astnodeclass, data, destructor);
	if (!obj) {
		free(data);
		return NULL;
	}
	return obj;
}
