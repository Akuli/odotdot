#include "classobject.h"

struct ObjectClassInfo *classobject_createclass(struct ObjectClassInfo *objectclass)
{
	return objectclassinfo_new(objectclass, NULL, NULL);
}

struct Object *classobject_newfromclassinfo(struct ObjectClassInfo *classobjectclass, struct ObjectClassInfo *wrapped)
{
	struct Object *klass = object_new(classobjectclass);
	if (!klass)
		return NULL;
	klass->data = wrapped;
	return klass;
}
