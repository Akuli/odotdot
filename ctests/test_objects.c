#include <src/interpreter.h>
#include <src/method.h>
#include <src/objects/array.h>
#include <src/objects/function.h>
#include <src/objects/integer.h>
#include <src/objects/mapping.h>
#include <src/objects/string.h>
#include <src/objectsystem.h>
#include <src/unicode.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include "utils.h"


#define DEADBEEFPTR ( (void*)(uintptr_t)0xdeadbeef )
#define ABCPTR ( (struct Object*)(uintptr_t)0xabc123 )

int cleaned = 0;
void cleaner(void *data)
{
	buttert(data == DEADBEEFPTR);
	cleaned++;
}

void test_objects_simple(void)
{
	buttert(cleaned == 0);
	struct Object *obj = object_new_noerr(testinterp, testinterp->builtins.Object, (struct ObjectData){.data=DEADBEEFPTR, .foreachref=NULL, .destructor=cleaner});
	buttert(obj);
	buttert(obj->objdata.data == DEADBEEFPTR);
	buttert(obj->objdata.foreachref == NULL);
	buttert(obj->objdata.destructor == cleaner);

	buttert(cleaned == 0);
	OBJECT_INCREF(testinterp, obj);
	OBJECT_DECREF(testinterp, obj);
	buttert(cleaned == 0);
	OBJECT_DECREF(testinterp, obj);
	buttert(cleaned == 1);
}


// free may be defined as a macro
void freee(void *ptr) { free(ptr); }

struct Object *callback_arg1, *callback_arg2;

struct Object *callback(struct Interpreter *interp, struct ObjectData xdata, struct Object *args, struct Object *opts)
{
	buttert(*((char*)xdata.data) == 'x');
	buttert(!xdata.foreachref);
	buttert(xdata.destructor == freee);
	buttert(MAPPINGOBJECT_SIZE(opts) == 0);
	buttert(interp == testinterp);
	buttert(args->klass == interp->builtins.Array);
	buttert(ARRAYOBJECT_LEN(args) == 2);
	buttert(ARRAYOBJECT_GET(args, 0) == callback_arg1);
	buttert(ARRAYOBJECT_GET(args, 1) == callback_arg2);
	return ABCPTR;
}

void test_objects_function(void)
{
	buttert((callback_arg1 = stringobject_newfromcharptr(testinterp, "asd1")));
	buttert((callback_arg2 = stringobject_newfromcharptr(testinterp, "asd2")));

	char *x = bmalloc(1);
	*x = 'x';
	struct Object *func = functionobject_new(testinterp, (struct ObjectData){.data=x, .foreachref=NULL, .destructor=freee}, functionobject_mkcfunc_yesret(callback), "test func");
	buttert(functionobject_call_yesret(testinterp, func, callback_arg1, callback_arg2, NULL) == ABCPTR);

	OBJECT_DECREF(testinterp, func);
	OBJECT_DECREF(testinterp, callback_arg1);
	OBJECT_DECREF(testinterp, callback_arg2);
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

	// u is used in 2 places here
	// newfromustr guarantees that u.val is eventually freed
	// newfromustr_copy copies u.val
	struct Object *strs[] = {
		stringobject_newfromcharptr(testinterp, "Öö"),
		stringobject_newfromustr_copy(testinterp, u),
		stringobject_newfromustr_copy(testinterp, u),
		stringobject_newfromustr_copy(testinterp, u),
		stringobject_newfromustr(testinterp, u),
	};

	for (size_t i=0; i < sizeof(strs)/sizeof(strs[0]); i++) {
		buttert(strs[i]);
		struct UnicodeString *data = strs[i]->objdata.data;
		buttert(data);
		buttert(data->len == 2);
		buttert(data->val[0] == ODOTDOT);
		buttert(data->val[1] == odotdot);
		OBJECT_DECREF(testinterp, strs[i]);
	}
}

void test_objects_string_newfromfmt(void)
{
	unicode_char bval = 'b';
	struct UnicodeString b = { .len = 1, .val = &bval };

	struct Object *c = stringobject_newfromcharptr(testinterp, "c");
	buttert(c);

	struct Object *i = integerobject_newfromlonglong(testinterp, 123);
	buttert(i);

	struct Object *res = stringobject_newfromfmt(testinterp, "-%s-%U-%S-%D-%L-%%-", "a", b, c, i, (long long) 123);
	buttert(res);
	OBJECT_DECREF(testinterp, c);
	OBJECT_DECREF(testinterp, i);

	char shouldB[] = "-a-b-c-123-123-%-";
	struct UnicodeString *s = res->objdata.data;
	buttert(s->len == strlen(shouldB));
	for (unsigned int i=0; i < s->len; i++)
		buttert(s->val[i] == (unsigned char) shouldB[i]);

	OBJECT_DECREF(testinterp, res);
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
		buttert((objs[i] = integerobject_newfromlonglong(testinterp, i+10)));

	struct Object *arr = arrayobject_new(testinterp, objs, HOW_MANY);
	check(arr->objdata.data);

	for (int i = HOW_MANY-1; i >= 0; i--) {
		struct Object *obj = arrayobject_pop(testinterp, arr);
		buttert(obj == objs[i]);
		OBJECT_DECREF(testinterp, obj);
		buttert(obj->refcount == 1);
	}

	for (size_t i=0; i < HOW_MANY; i++)
		buttert(arrayobject_push(testinterp, arr, objs[i]) == true);
	check(arr->objdata.data);

	OBJECT_DECREF(testinterp, arr);
	for (size_t i=0; i < HOW_MANY; i++)
		OBJECT_DECREF(testinterp, objs[i]);
}
#undef HOW_MANY

#define HUGE 1234
void test_objects_mapping_huge(void)
{
	struct Object *keys[HUGE];
	struct Object *vals[HUGE];
	for (int i=0; i < HUGE; i++) {
		buttert((keys[i] = integerobject_newfromlonglong(testinterp, i)));
		buttert((vals[i] = integerobject_newfromlonglong(testinterp, -i)));
	}

	struct Object *map = mappingobject_newempty(testinterp);
	buttert(map);

	int counter = 3;
	while (counter--) {    // repeat 3 times
		for (int i=0; i < HUGE; i++) {
			// do it twice to make sure that second call does nothing
			buttert(method_call_noret(testinterp, map, "set", keys[i], vals[i], NULL));
			buttert(method_call_noret(testinterp, map, "set", keys[i], vals[i], NULL));
		}
		buttert(((struct MappingObjectData *) map->objdata.data)->size == HUGE);

		for (int i=0; i < HUGE; i++)
			buttert(method_call_noret(testinterp, map, "delete", keys[i], NULL));
		buttert(((struct MappingObjectData *) map->objdata.data)->size == 0);
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
#define I(x) integerobject_newfromlonglong(testinterp, (x))
	struct Object* keys[] = { I(0), I(1), I(2) };
	struct Object* vals[] = { I(100), I(101), I(102) };
#undef I
#define FINAL_SIZE (sizeof(keys) / sizeof(keys[0]))
	int found[FINAL_SIZE] = {0};

	struct Object *map = mappingobject_newempty(testinterp);
	buttert(map);
	for (unsigned int i=0; i < FINAL_SIZE; i++){
		buttert(keys[i]);
		buttert(vals[i]);
		buttert(mappingobject_set(testinterp, map, keys[i], vals[i]));
		OBJECT_DECREF(testinterp, keys[i]);
		OBJECT_DECREF(testinterp, vals[i]);
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


// classobject isn't tested here because it's used a lot when setting up testinterp
