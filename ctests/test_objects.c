#include <src/common.h>
#include <src/interpreter.h>
#include <src/method.h>
#include <src/objects/array.h>
#include <src/objects/classobject.h>
#include <src/objects/errors.h>
#include <src/objects/function.h>
#include <src/objects/integer.h>
#include <src/objects/mapping.h>
#include <src/objects/string.h>
#include <src/objectsystem.h>
#include <src/unicode.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "utils.h"


int cleaner_ran = 0;
void cleaner(struct Object *obj)
{
	buttert(obj->data == (void *)0xdeadbeef);
	cleaner_ran++;
}

void test_objects_simple(void)
{
	buttert(cleaner_ran == 0);
	struct Object *obj = classobject_newinstance(testinterp, &testerr, testinterp->builtins.objectclass, (void *)0xdeadbeef, cleaner);
	buttert(obj);
	buttert(obj->data == (void *)0xdeadbeef);
	buttert(cleaner_ran == 0);
	OBJECT_DECREF(testinterp, obj);
	buttert(cleaner_ran == 1);
}

void test_objects_error(void)
{
	struct Object *err = NULL;
	errorobject_setwithfmt(testinterp, &err, "oh %s", "shit");
	buttert(err);
	struct UnicodeString *msg = ((struct Object*) err->data)->data;
	buttert(msg);
	buttert(msg->len = 7);
	buttert(
		msg->val[0] == 'o' &&
		msg->val[1] == 'h' &&
		msg->val[2] == ' ' &&
		msg->val[3] == 's' &&
		msg->val[4] == 'h' &&
		msg->val[5] == 'i' &&
		msg->val[6] == 't');
	OBJECT_DECREF(testinterp, err);
}

struct Object *callback_arg1, *callback_arg2;
int flipped = 0;

struct Object *callback(struct Interpreter *interp, struct Object **errptr, struct Object **args, size_t nargs)
{
	buttert(nargs == 2);
	buttert(args[0] == (flipped ? callback_arg2 : callback_arg1));
	buttert(args[1] == (flipped ? callback_arg1 : callback_arg2));
	return (struct Object*) 0x123abc;
}

void test_objects_function(void)
{
	buttert((callback_arg1 = stringobject_newfromcharptr(testinterp, &testerr, "asd1")));
	buttert((callback_arg2 = stringobject_newfromcharptr(testinterp, &testerr, "asd2")));

	struct Object *func = functionobject_new(testinterp, &testerr, callback);
	buttert(functionobject_call(testinterp, &testerr, func, callback_arg1, callback_arg2, NULL) == (struct Object*) 0x123abc);

	struct Object *partial1 = functionobject_newpartial(testinterp, &testerr, func, callback_arg1);
	OBJECT_DECREF(testinterp, callback_arg1);   // partialfunc should hold a reference to this
	OBJECT_DECREF(testinterp, func);
	flipped = 0;
	buttert(functionobject_call(testinterp, &testerr, partial1, callback_arg2, NULL) == (struct Object*) 0x123abc);

	struct Object *partial2 = functionobject_newpartial(testinterp, &testerr, partial1, callback_arg2);
	OBJECT_DECREF(testinterp, callback_arg2);
	OBJECT_DECREF(testinterp, partial1);
	flipped = 1;    // arg 2 was partialled last, so it will go first
	buttert(functionobject_call(testinterp, &testerr, partial2, NULL) == (struct Object*) 0x123abc);

	OBJECT_DECREF(testinterp, partial2);
}

#define ODOTDOT 0xd6    // Ö
#define odotdot 0xf6    // ö

void test_objects_string(void)
{
	struct UnicodeString u;
	u.len = 2;
	u.val = bmalloc(sizeof(unicode_char) * 2);
	u.val[0] = ODOTDOT;
	u.val[1] = odotdot;

	struct Object *strs[] = {
		stringobject_newfromcharptr(testinterp, &testerr, "Öö"),
		stringobject_newfromustr(testinterp, &testerr, u) };
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
	struct Object *s = stringobject_newfromcharptr(testinterp, &testerr, "Öö");
	buttert(s);
	struct Object *ret = method_call(testinterp, &testerr, s, "to_string", NULL);
	buttert(ret);
	buttert(ret == s);
	OBJECT_DECREF(testinterp, ret);
	OBJECT_DECREF(testinterp, s);    // stringobject_newfromustr() returned a new reference
}

void test_objects_string_newfromfmt(void)
{
	unicode_char bval = 'b';
	struct UnicodeString b;
	b.len = 1;
	b.val = &bval;

	struct Object *c = stringobject_newfromcharptr(testinterp, &testerr, "c");
	buttert(c);

	struct Object *res = stringobject_newfromfmt(testinterp, &testerr, "-%s-%U-%S-%D-%%-", "a", b, c, c);
	buttert(res);
	OBJECT_DECREF(testinterp, c);

	struct UnicodeString *s = res->data;
	buttert(s->len == 13 /* OMG BAD LUCK */);
	buttert(
		s->val[0] == '-' &&
		s->val[1] == 'a' &&
		s->val[2] == '-' &&
		s->val[3] == 'b' &&
		s->val[4] == '-' &&
		s->val[5] == 'c' &&
		s->val[6] == '-' &&
		s->val[7] == '"' &&
		s->val[8] == 'c' &&
		s->val[9] == '"' &&
		s->val[10] == '-' &&
		s->val[11] == '%' &&
		s->val[12] == '-');
	OBJECT_DECREF(testinterp, res);
}

void test_objects_array(void)
{
	struct Object *objs[] = {
		stringobject_newfromcharptr(testinterp, &testerr, "a"),
		stringobject_newfromcharptr(testinterp, &testerr, "b"),
		stringobject_newfromcharptr(testinterp, &testerr, "c") };
#define NOBJS (sizeof(objs) / sizeof(objs[0]))
	for (size_t i=0; i < NOBJS; i++)
		buttert(objs[i]);

	struct Object *arr = arrayobject_new(testinterp, &testerr, objs, NOBJS - 1);
	buttert(arr);
	buttert(arrayobject_push(testinterp, &testerr, arr, objs[NOBJS-1]) == STATUS_OK);

	// now the array should hold references to each object
	for (size_t i=0; i < NOBJS; i++)
		OBJECT_DECREF(testinterp, objs[i]);

	struct ArrayObjectData *data = arr->data;
	buttert(data->len == NOBJS);
	for (size_t i=0; i < NOBJS; i++) {
#undef NOBJS
		buttert(data->elems[i] == objs[i]);
		struct UnicodeString *ustr = ((struct Object *) data->elems[i])->data;
		buttert(ustr->len == 1);
		buttert(ustr->val[0] == 'a' + i);
	}
	OBJECT_DECREF(testinterp, arr);
}

#define HOW_MANY 1000
static void check(struct ArrayObjectData *arrdata)
{
	buttert(arrdata);
	buttert(arrdata->len == HOW_MANY);
	for (size_t i=0; i < HOW_MANY; i++) {
		buttert(arrdata->elems[i]->refcount == 2);
		buttert(integerobject_tolonglong(arrdata->elems[i]) == (long long) (i + 10));
	}
}

void test_objects_array_many_elems(void)
{
	struct Object *objs[HOW_MANY];
	for (size_t i=0; i < HOW_MANY; i++)
		buttert((objs[i] = integerobject_newfromlonglong(testinterp, &testerr, i+10)));

	struct Object *arr = arrayobject_new(testinterp, &testerr, objs, HOW_MANY);
	check(arr->data);

	for (int i = HOW_MANY-1; i >= 0; i--) {
		struct Object *obj = arrayobject_pop(testinterp, arr);
		buttert(obj == objs[i]);
		OBJECT_DECREF(testinterp, obj);
		buttert(obj->refcount == 1);
	}

	for (size_t i=0; i < HOW_MANY; i++)
		buttert(arrayobject_push(testinterp, &testerr, arr, objs[i]) == STATUS_OK);
	check(arr->data);

	OBJECT_DECREF(testinterp, arr);
	for (size_t i=0; i < HOW_MANY; i++)
		OBJECT_DECREF(testinterp, objs[i]);
}
#undef HOW_MANY

void test_objects_mapping(void)
{
#define S(x) stringobject_newfromcharptr(testinterp, &testerr, (x))
	struct Object* keys[] = { S("a"), S("b"), S("c") };
	struct Object* vals[] = { S("x"), S("y"), S("z") };
#undef S
#define FINAL_SIZE (sizeof(keys) / sizeof(keys[0]))

	for (unsigned int i=0; i < FINAL_SIZE; i++) {
		buttert(keys[i]);
		buttert(vals[i]);
	}

	struct Object *map = mappingobject_newempty(testinterp, &testerr);
	buttert(map);
	buttert(((struct MappingObjectData *) map->data)->size == 0);

	// adding non-hashable keys must fail, but non-hashable values are ok
	struct Object *yeshash = stringobject_newfromcharptr(testinterp, &testerr, "lol asd wut wat");
	struct Object *nohash = arrayobject_newempty(testinterp, &testerr);
	struct Object *err = NULL;
	struct Object *ret;

	buttert(!method_call(testinterp, &err, map, "set", nohash, yeshash, NULL));
	buttert(err);
	OBJECT_DECREF(testinterp, err);
	err = NULL;

	buttert((ret = method_call(testinterp, &err, map, "set", yeshash, nohash, NULL)));
	buttert(!err);
	OBJECT_DECREF(testinterp, ret);
	OBJECT_DECREF(testinterp, yeshash);
	OBJECT_DECREF(testinterp, nohash);

	for (unsigned int i=0; i < FINAL_SIZE; i++) {
		// test trying to get a missing item
		buttert(!method_call(testinterp, &err, map, "get", keys[i], NULL));
		buttert(err);
		OBJECT_DECREF(testinterp, err);
		err = NULL;

		ret = method_call(testinterp, &testerr, map, "set", keys[i], vals[i], NULL);
		buttert(ret);
		OBJECT_DECREF(testinterp, ret);

		// must work the same
		ret = method_call(testinterp, &testerr, map, "set", keys[i], vals[i], NULL);
		buttert(ret);
		OBJECT_DECREF(testinterp, ret);
	}

	// it's ok to not delete everything from the mapping
	ret = method_call(testinterp, &testerr, map, "get_and_delete", keys[0], NULL);
	buttert(ret == vals[0]);
	OBJECT_DECREF(testinterp, ret);

	ret = method_call(testinterp, &testerr, map, "delete", keys[1], NULL);
	buttert(ret);
	buttert(ret != vals[1]);   // TODO: check that it's null when null is implemented
	OBJECT_DECREF(testinterp, ret);

	OBJECT_DECREF(testinterp, map);
	for (unsigned int i=0; i < FINAL_SIZE; i++) {
		OBJECT_DECREF(testinterp, keys[i]);
		OBJECT_DECREF(testinterp, vals[i]);
	}
#undef FINAL_SIZE
}

#define HUGE 1234
void test_objects_mapping_huge(void)
{
	struct Object *keys[HUGE];
	struct Object *vals[HUGE];
	for (int i=0; i < HUGE; i++) {
		buttert((keys[i] = integerobject_newfromlonglong(testinterp, &testerr, i)));
		buttert((vals[i] = integerobject_newfromlonglong(testinterp, &testerr, -i)));
	}

	struct Object *map = mappingobject_newempty(testinterp, &testerr);
	buttert(map);

	int counter = 3;
	while (counter--) {    // repeat 3 times
		struct Object *ret;
		for (int i=0; i < HUGE; i++) {
			ret = method_call(testinterp, &testerr, map, "set", keys[i], vals[i], NULL);
			buttert(ret);
			OBJECT_DECREF(testinterp, ret);

			// do it again :D this should do nothing
			ret = method_call(testinterp, &testerr, map, "set", keys[i], vals[i], NULL);
			buttert(ret);
			OBJECT_DECREF(testinterp, ret);
		}
		buttert(((struct MappingObjectData *) map->data)->size == HUGE);

		for (int i=0; i < HUGE; i++) {
			ret = method_call(testinterp, &testerr, map, "delete", keys[i], NULL);
			buttert(ret);
			OBJECT_DECREF(testinterp, ret);
		}
		buttert(((struct MappingObjectData *) map->data)->size == 0);
	}

	OBJECT_DECREF(testinterp, map);
	for (size_t i=0; i < HUGE; i++)
	{
		OBJECT_DECREF(testinterp, keys[i]);
		OBJECT_DECREF(testinterp, vals[i]);
	}
}
#undef HUGE

void test_objects_mapping_iter(void)
{
#define I(x) integerobject_newfromlonglong(testinterp, &testerr, (x))
	struct Object* keys[] = { I(0), I(1), I(2) };
	struct Object* vals[] = { I(100), I(101), I(102) };
#undef I
#define FINAL_SIZE (sizeof(keys) / sizeof(keys[0]))
	int found[FINAL_SIZE] = {0};

	struct Object *map = mappingobject_newempty(testinterp, &testerr);
	buttert(map);
	for (unsigned int i=0; i < FINAL_SIZE; i++){
		struct Object *ret = method_call(testinterp, &testerr, map, "set", keys[i], vals[i], NULL);
		OBJECT_DECREF(testinterp, keys[i]);
		OBJECT_DECREF(testinterp, vals[i]);
		OBJECT_DECREF(testinterp, ret);
	}

	struct MappingObjectIter iter;
	mappingobject_iterbegin(&iter, map);
	while (mappingobject_iternext(&iter)) {
		int k = integerobject_tolonglong(iter.key), v = integerobject_tolonglong(iter.value);
		buttert(k + 100 == v);
		buttert(!found[k]);
		found[k] = 1;
	}

	for (unsigned int i=0; i < FINAL_SIZE; i++)
		buttert(found[i]);

	OBJECT_DECREF(testinterp, map);
}


struct HashTest {
	struct Object *obj;
	int shouldBhashable;
};

void test_objects_hashes(void)
{
	struct Object *err;
	errorobject_setwithfmt(testinterp, &err, "oh %s", "shit");

	OBJECT_INCREF(testinterp, testinterp->builtins.stringclass);
	OBJECT_INCREF(testinterp, testinterp->builtins.print);
	struct HashTest tests[] = {
		{ testinterp->builtins.stringclass, 1 },
		{ err, 1 },
		{ testinterp->builtins.print, 1 },
		{ integerobject_newfromlonglong(testinterp, &testerr, -123LL), 1 },
		{ classobject_newinstance(testinterp, &testerr, testinterp->builtins.objectclass, NULL, NULL), 1 },
		{ stringobject_newfromcharptr(testinterp, &testerr, "asd"), 1 },
		{ arrayobject_newempty(testinterp, &testerr), 0 },
		{ mappingobject_newempty(testinterp, &testerr), 0 }
	};
	err = NULL;

	for (unsigned int i=0; i < sizeof(tests) / sizeof(tests[0]); i++) {
		struct HashTest test = tests[i];
		buttert(test.obj);
		buttert(test.shouldBhashable == !!test.shouldBhashable);
		if (test.shouldBhashable) {
			buttert(test.obj->hashable == 1);

			// just to make sure that the hash is accessed and valgrind complains if it's not set
			// if you're really unlucky, this fails when things actually work
			buttert(test.obj->hash != 123456);
		} else {
			buttert(test.obj->hashable == 0);
			// don't access test.obj.hash here
		}

		OBJECT_DECREF(testinterp, test.obj);
	}
}


// classobject isn't tested here because it's used a lot when setting up testinterp
