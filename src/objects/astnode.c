#include "astnode.h"
#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include "../interpreter.h"
#include "../method.h"
#include "../objectsystem.h"
#include "../tokenizer.h"
#include "../unicode.h"
#include "array.h"
#include "classobject.h"
#include "errors.h"
#include "integer.h"
#include "string.h"

static void astnode_foreachref(struct Object *node, void *cbdata, object_foreachrefcb cb)
{
	struct AstNodeObjectData *data = node->data;
	switch (data->kind) {
#define info_as(X) ((struct X *) data->info)
	case AST_ARRAY:
	case AST_BLOCK:
		cb(info_as(AstArrayOrBlockInfo), cbdata);
		break;
	case AST_GETATTR:
		cb(info_as(AstGetAttrInfo)->objnode, cbdata);
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
		cb(info_as(AstCallInfo)->args, cbdata);
		cb(info_as(AstCallInfo)->opts, cbdata);
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
	struct AstNodeObjectData *data = node->data;
	switch (data->kind) {
#define info_as(X) ((struct X *) data->info)
	case AST_GETVAR:
		free(info_as(AstGetVarInfo)->varname.val);
		free(data->info);
		break;
	case AST_GETATTR:
		free(info_as(AstGetAttrInfo)->name.val);
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

	free(data->filename);
	free(data);
}

static struct Object *setup(struct Interpreter *interp, struct Object *args, struct Object *opts)
{
	errorobject_throwfmt(interp, "TypeError", "cannot create new AstNode objects");
	return NULL;
}

struct Object *astnodeobject_createclass(struct Interpreter *interp)
{
	// the 1 means that AstNode instances may have attributes
	// TODO: add at least kind and lineno attributes to the nodes?
	struct Object *klass = classobject_new(interp, "AstNode", interp->builtins.Object, false);
	if (!klass)
		return NULL;

	if (!method_add(interp, klass, "setup", setup)) goto error;
	return klass;

error:
	OBJECT_DECREF(interp, klass);
	return NULL;
}

struct Object *astnodeobject_new(struct Interpreter *interp, char kind, char *filename, size_t lineno, void *info)
{
	struct AstNodeObjectData *data = malloc(sizeof(struct AstNodeObjectData));
	if (!data) {
		errorobject_thrownomem(interp);
		return NULL;
	}

	size_t len = strlen(filename);
	if (!(data->filename = malloc(len+1))) {
		free(data);
		errorobject_thrownomem(interp);
		return NULL;
	}
	memcpy(data->filename, filename, len+1);

	data->kind = kind;
	data->lineno = lineno;
	data->info = info;

	struct Object *obj = classobject_newinstance(interp, interp->builtins.AstNode, data, astnode_foreachref, astnode_destructor);
	if (!obj) {
		free(data->filename);
		free(data);
		return NULL;
	}
	return obj;
}
