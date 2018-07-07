#include "astnode.h"
#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include "../interpreter.h"
#include "../objectsystem.h"
#include "classobject.h"
#include "errors.h"

static void astnode_foreachref(void *data, object_foreachrefcb cb, void *cbdata)
{
	switch (((struct AstNodeObjectData *)data)->kind) {
#define info_as(X) ((struct X *) ((struct AstNodeObjectData *)data)->info)
	case AST_ARRAY:
	case AST_BLOCK:
	case AST_INT:
	case AST_STR:
		cb(info_as(AstArrayOrBlockInfo), cbdata);
		break;
	case AST_GETATTR:
		cb(info_as(AstGetAttrInfo)->objnode, cbdata);
		cb(info_as(AstGetAttrInfo)->name, cbdata);
		break;
	case AST_CREATEVAR:
	case AST_SETVAR:
		cb(info_as(AstCreateOrSetVarInfo)->valnode, cbdata);
		cb(info_as(AstCreateOrSetVarInfo)->varname, cbdata);
		break;
	case AST_SETATTR:
		cb(info_as(AstSetAttrInfo)->objnode, cbdata);
		cb(info_as(AstSetAttrInfo)->attr, cbdata);
		cb(info_as(AstSetAttrInfo)->valnode, cbdata);
		break;
	case AST_CALL:
		cb(info_as(AstCallInfo)->funcnode, cbdata);
		cb(info_as(AstCallInfo)->args, cbdata);
		cb(info_as(AstCallInfo)->opts, cbdata);
		break;
	case AST_OPCALL:
		cb(info_as(AstOpCallInfo)->lhs, cbdata);
		cb(info_as(AstOpCallInfo)->rhs, cbdata);
		break;
	case AST_GETVAR:
		cb(info_as(AstGetVarInfo)->varname, cbdata);
		break;
#undef info_as
	default:
		assert(0);      // unknown kind
	}
}

static void astnode_destructor(void *data)
{
	switch (((struct AstNodeObjectData *)data)->kind) {
#define info_as(X) ((struct X *) ((struct AstNodeObjectData *)data)->info)
	case AST_GETVAR:
	case AST_GETATTR:
	case AST_CREATEVAR:
	case AST_SETVAR:
	case AST_CALL:
	case AST_OPCALL:
	case AST_SETATTR:
		free(((struct AstNodeObjectData *)data)->info);
		break;
	case AST_INT:
	case AST_STR:
	case AST_ARRAY:
	case AST_BLOCK:
		// do nothing, the info is an Object handled by astnode_foreachref
		break;
#undef info_as
	default:
		assert(0);  // unknown kind
	}

	free(((struct AstNodeObjectData *)data)->filename);
	free(data);
}

static struct Object *newinstance(struct Interpreter *interp, struct Object *args, struct Object *opts)
{
	errorobject_throwfmt(interp, "TypeError", "new AstNode objects cannot be created yet, sorry :(");
	return NULL;
}

struct Object *astnodeobject_createclass(struct Interpreter *interp)
{
	// TODO: add at least kind and lineno attributes to the nodes?
	return classobject_new(interp, "AstNode", interp->builtins.Object, newinstance);
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

	struct Object *obj = object_new_noerr(interp, interp->builtins.AstNode, (struct ObjectData){.data=data, .foreachref=astnode_foreachref, .destructor=astnode_destructor});
	if (!obj) {
		errorobject_thrownomem(interp);
		free(data->filename);
		free(data);
		return NULL;
	}
	return obj;
}
