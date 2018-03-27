#include <src/common.h>
#include <src/hashtable.h>
#include <src/objects/function.h>
#include <src/objects/object.h>
#include <src/objects/string.h>
#include <src/objectsystem.h>
#include <stddef.h>
#include "utils.h"

struct ObjectClassInfo *objectclass, *functionclass, *stringclass;


void objects_test_setup(void)
{
	buttert(objectclass = objectobject_createclass());
	buttert(functionclass = functionobject_createclass(objectclass));
	buttert(stringclass = stringobject_createclass(objectclass));
}
void objects_test_teardown(void)
{
	objectclassinfo_free(stringclass);
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

#define ODOTDOT 0xd6    // Ö
#define odotdot 0xf6    // ö

void test_objects_string(void)
{
	struct UnicodeString u;
	u.len = 2;
	u.val = bmalloc(sizeof(uint32_t) * 2);
	u.val[0] = ODOTDOT;
	u.val[1] = odotdot;

	struct Object *strs[] = {
		stringobject_newfromcharptr(stringclass, "Öö"),
		stringobject_newfromustr(stringclass, u) };
	free(u.val);    // must not break anything, newfromustr should copy

	for (size_t i=0; i < sizeof(strs)/sizeof(strs[0]); i++) {
		buttert(strs[i]);
		struct UnicodeString *data = strs[i]->data;
		buttert(data);
		buttert(data->len == 2);
		buttert(data->val[0] == ODOTDOT);
		buttert(data->val[1] == odotdot);
		object_free(strs[i]);
	}
}
