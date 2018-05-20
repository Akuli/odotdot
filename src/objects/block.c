#include "block.h"
#include "../attribute.h"
#include "../common.h"
#include "../interpreter.h"   // IWYU pragma: keep
#include "../objectsystem.h"  // IWYU pragma: keep
#include "array.h"
#include "classobject.h"


struct Object *blockobject_createclass(struct Interpreter *interp, struct Object **errptr)
{
	// TODO: run method
	return classobject_new(interp, errptr, "Block", interp->builtins.objectclass, 1, NULL);
}

struct Object *blockobject_new(struct Interpreter *interp, struct Object **errptr, struct Object *definitionscope, struct Object *astnodearr)
{
	struct Object *block = classobject_newinstance(interp, errptr, interp->builtins.blockclass, NULL, NULL);
	if (!block)
		return NULL;

	if (attribute_set(interp, errptr, block, "definition_scope", definitionscope) == STATUS_ERROR) goto error;
	if (attribute_set(interp, errptr, block, "ast_statements", astnodearr) == STATUS_ERROR) goto error;
	return block;

error:
	OBJECT_DECREF(interp, block);
	return NULL;
}
