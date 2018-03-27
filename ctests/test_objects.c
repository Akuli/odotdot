#include <src/common.h>
#include <src/hashtable.h>
#include <src/objects/function.h>
#include <src/objects/object.h>
#include <src/objectsystem.h>
#include <stddef.h>
#include "utils.h"

struct ObjectClassInfo *objectclass, *functionclass;


void objects_test_setup(void)
{
	buttert(objectclass = objectobject_createclass());
	buttert(functionclass = functionobject_createclass(objectclass));
}
void objects_test_teardown(void)
{
	objectclassinfo_free(functionclass);
	objectclassinfo_free(objectclass);
}


// TODO: is this needed?
void test_objects_objectclass_stuff(void)
{
	buttert(objectclass->baseclass == NULL);
	buttert(objectclass->methods);
	buttert(objectclass->methods->size == 0);
	buttert(objectclass->destructor == NULL);
}

void test_objects_simple(void)
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

void test_objects_function(void)
{
	callback_ran = 0;
	struct Object *func = functionobject_new(functionclass, callback);
	buttert(!callback_ran);
	functionobject_get_cfunc(func)(NULL);
	buttert(callback_ran);
	object_free(func);
}
