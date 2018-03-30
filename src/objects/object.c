#include "object.h"
#include <stdlib.h>
#include "../objectsystem.h"


struct ObjectClassInfo *objectobject_createclass(void)
{
	return objectclassinfo_new(NULL, NULL, NULL);
}
