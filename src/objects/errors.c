#include "errors.h"
#include <stdlib.h>
#include "../objectsystem.h"
#include "string.h"

static int error_destructor(struct Object *err)
{
	return object_free(err->data);    // TODO: replace this with a decref or something?
}

struct ObjectClassInfo *errorobject_createclass(struct ObjectClassInfo *objectclass)
{
	return objectclassinfo_new(objectclass, error_destructor);
}

struct Object *errorobject_newfromstringobject(struct ObjectClassInfo *errorclass, struct Object *msgstring)
{
	struct Object *err = object_new(errorclass);
	if (!err)
		return NULL;
	err->data = msgstring;
	return err;
}

struct Object *errorobject_newfromcharptr(struct ObjectClassInfo *errorclass, struct ObjectClassInfo *stringclass, char *msg)
{
	struct Object *msgobj = stringobject_newfromcharptr(stringclass, msg);
	if(!msgobj)
		return NULL;

	struct Object *err = errorobject_newfromstringobject(errorclass, msgobj);
	if(!err) {
		free(msgobj);
		return NULL;
	}

	return err;
}
