#include <src/dynamicarray.h>
#include <src/hashtable.h>
#include <src/interpreter.h>
#include <src/objects/array.h>
#include <src/objects/classobject.h>
#include <src/objects/function.h>
#include <src/objects/integer.h>
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
	struct Object *objectclass = interpreter_getbuiltin(testinterp, NULL, "Object");
	struct ObjectClassInfo *objectinfo = objectclass->data;
	OBJECT_DECREF(testinterp, objectclass);

	buttert(objectinfo->baseclass == NULL);
	buttert(objectinfo->methods);
	buttert(objectinfo->methods->size == 0);
	buttert(objectinfo->foreachref == NULL);
	buttert(objectinfo->destructor == NULL);
}

void test_objects_simple(void)
{
	struct Object *objectclass = interpreter_getbuiltin(testinterp, NULL, "Object");
	struct Object *obj = classobject_newinstance(testinterp, NULL, objectclass, (void *)0xdeadbeef);
	OBJECT_DECREF(testinterp, objectclass);
	buttert(obj);
	buttert(obj->data == (void *)0xdeadbeef);
	OBJECT_DECREF(testinterp, obj);
}


// TODO: test actually running this thing to make sure that data is passed correctly
struct Object *callback(struct Context *callctx, struct Object **errptr, struct Object **args, size_t nargs)
{
	buttert2(0, "the callback ran unexpectedly");
	return (struct Object*) 0xdeadbeef;
}

void test_objects_function(void)
{
	struct Object *func = functionobject_new(testinterp, NULL, callback, NULL);
	buttert(functionobject_getcfunc(testinterp, func) == callback);
	OBJECT_DECREF(testinterp, func);
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
		OBJECT_DECREF(testinterp, strs[i]);
	}
}

void test_objects_string_tostring(void)
{
	struct Object *s = stringobject_newfromcharptr(testinterp, NULL, "Öö");
	buttert(s);
	struct Object *tostring = classobject_getmethod(testinterp, NULL, s, "to_string");
	buttert(tostring);
	struct Object *ret = functionobject_call(testinterp->builtinctx, NULL, tostring, NULL);
	OBJECT_DECREF(testinterp, tostring);

	buttert(ret == s);
	OBJECT_DECREF(testinterp, s);    // functionobject_call() returned a new reference
	OBJECT_DECREF(testinterp, s);    // stringobject_newfromustr() returned a new reference
}

#define NOBJS 3
void test_objects_array(void)
{
	struct Object *objs[] = {
		stringobject_newfromcharptr(testinterp, NULL, "a"),
		stringobject_newfromcharptr(testinterp, NULL, "b"),
		stringobject_newfromcharptr(testinterp, NULL, "c") };
	for (int i=0; i < NOBJS; i++)
		buttert(objs[i]);

	struct Object *arr = arrayobject_new(testinterp, NULL, objs, NOBJS);
	buttert(arr);
	// now the array should hold references to each object
	for (int i=0; i < NOBJS; i++)
		OBJECT_DECREF(testinterp, objs[i]);

	struct DynamicArray *dynarray = arr->data;
	buttert(dynarray->len == NOBJS);
	for (int i=0; i < NOBJS; i++) {
		buttert(dynarray->values[i] == objs[i]);
		struct UnicodeString *ustr = ((struct Object *) dynarray->values[i])->data;
		buttert(ustr->len == 1);
		buttert((int) ustr->val[0] == 'a' + i);
	}
	OBJECT_DECREF(testinterp, arr);
}
#undef NOBJS

void test_objects_integer(void)
{
	struct UnicodeString u;
	u.len = 5;
	u.val = bmalloc(sizeof(uint32_t) * 5);
	u.val[0] = '-';
	u.val[1] = '0';     // leading zeros don't matter
	u.val[2] = '1';
	u.val[3] = '2';
	u.val[4] = '3';

	struct Object *negints[] = {
		integerobject_newfromustr(testinterp, NULL, u),
		integerobject_newfromcharptr(testinterp, NULL, "-0123") };

	// skip the minus sign
	u.len--;
	u.val++;

	struct Object *posints[] = {
		integerobject_newfromustr(testinterp, NULL, u),
		integerobject_newfromcharptr(testinterp, NULL, "0123") };
	free(u.val - 1 /* undo the minus skip */);

	buttert(sizeof(posints) == sizeof(negints));

	for (size_t i=0; i < sizeof(posints)/sizeof(posints[0]); i++) {
		buttert(posints[i]);
		buttert(negints[i]);
		buttert(integerobject_toint64(posints[i]) == 123);
		buttert(integerobject_toint64(negints[i]) == -123);
		OBJECT_DECREF(testinterp, posints[i]);
		OBJECT_DECREF(testinterp, negints[i]);
	}

	// "0" and "-0" should be treated equally
	struct Object *zero1 = integerobject_newfromcharptr(testinterp, NULL, "0");
	struct Object *zero2 = integerobject_newfromcharptr(testinterp, NULL, "-0");
	buttert(integerobject_toint64(zero1) == 0);
	buttert(integerobject_toint64(zero2) == 0);
	OBJECT_DECREF(testinterp, zero1);
	OBJECT_DECREF(testinterp, zero2);

	// biggest and smallest allowed values
	// TODO: implement and test error handling for too big or small values
	struct Object *big = integerobject_newfromcharptr(testinterp, NULL, "9223372036854775807");
	struct Object *small = integerobject_newfromcharptr(testinterp, NULL, "-9223372036854775808");
	buttert(integerobject_toint64(big) == INT64_MAX);
	buttert(integerobject_toint64(small) == INT64_MIN);
	OBJECT_DECREF(testinterp, big);
	OBJECT_DECREF(testinterp, small);
}

// classobject isn't tested here because it's used a lot when setting up testinterp
