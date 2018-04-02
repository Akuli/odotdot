#include <src/hashtable.h>
#include <src/interpreter.h>
#include <src/objects/function.h>
#include <src/objects/string.h>
#include <src/objectsystem.h>
#include <src/unicode.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include "utils.h"


// TODO: get rid of these
struct ObjectClassInfo *stringclass = NULL;
struct ObjectClassInfo *objectclass = NULL;
struct ObjectClassInfo *functionclass = NULL;

// TODO: is this needed?
void test_objects_objectclass_stuff(void)
{
	struct ObjectClassInfo *objectinfo = interpreter_getbuiltin(testinterp, NULL, "Object")->data;
	buttert(objectinfo->baseclass == NULL);
	buttert(objectinfo->methods);
	buttert(objectinfo->methods->size == 0);
	buttert(objectinfo->foreachref == NULL);
	buttert(objectinfo->destructor == NULL);
}

void test_objects_simple(void)
{
	struct ObjectClassInfo *objectinfo = interpreter_getbuiltin(testinterp, NULL, "Object")->data;
	struct Object *obj = object_new(objectinfo, (void *)0xdeadbeef);
	buttert(obj);
	buttert(obj->data == (void *)0xdeadbeef);
	object_free(obj);
}


struct Object *callback(struct Context *callctx, struct Object **errptr, struct DynamicArray *args)
{
	buttert2(0, "the callback ran unexpectedly");
	return (struct Object*) 0xdeadbeef;
}

void test_objects_function(void)
{
	struct Object *func = functionobject_new(testinterp, NULL, callback);
	buttert(functionobject_getcfunc(testinterp, NULL, func) == callback);
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
		stringobject_newfromcharptr(testinterp, NULL, "Öö"),
		stringobject_newfromustr(testinterp, NULL, u) };
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

// classobject isn't tested here because setup code in run.c uses it a lot anyway
