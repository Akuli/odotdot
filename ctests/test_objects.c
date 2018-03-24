#include "utils.h"
#include <src/objectsystem.h>
#include <src/objects/object.h>

struct ObjectClassInfo *objectclass;


// TODO: is this needed?
void test_objectclass_stuff(void)
{
	buttert(objectclass->baseclass == NULL);
	buttert(objectclass->methods);
	buttert(objectclass->methods->size == 0);
	buttert(objectclass->destructor == NULL);
}


void test_simple_object(void)
{
	struct Object *obj = object_new(objectclass);
	buttert(obj);
	object_free(obj);
}


int main(int argc, char **argv)
{
	objectclass = objectobject_createclass();
	buttert(objectclass);

	RUN_TESTS(argc, argv, test_objectclass_stuff, test_simple_object);
	objectclassinfo_free(objectclass);
}
