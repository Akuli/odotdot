#include "object.h"
#include "../objectsystem.h"

struct ObjectClassInfo *objectobject_createclass(void)
{
	return objectclassinfo_new(NULL, NULL);
}
