#include "utils.h"
#include <src/common.h>
#include <src/hashtable.h>
#include <src/objectsystem.h>
#include <src/objects/object.h>
#include <src/objects/function.h>

struct ObjectClassInfo *objectclass, *functionclass;


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


int callback_ran;
int callback(struct Object **retval)
{
	callback_ran = 1;
	return STATUS_OK;
}

void test_function_object(void)
{
	callback_ran = 0;
	struct Object *func = functionobject_new(functionclass, callback);
	buttert(!callback_ran);
	functionobject_get_cfunc(func)(NULL);
	buttert(callback_ran);
	object_free(func);
}


int main(int argc, char **argv)
{
	buttert(objectclass = objectobject_createclass());
	buttert(functionclass = functionobject_createclass(objectclass));
	RUN_TESTS(argc, argv, test_objectclass_stuff, test_simple_object, test_function_object);
	objectclassinfo_free(objectclass);
	objectclassinfo_free(functionclass);
}
