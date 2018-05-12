#include <src/common.h>
#include <src/interpreter.h>
#include <src/method.h>
#include <src/objects/array.h>
#include <src/objects/classobject.h>
#include <src/objects/errors.h>
#include <src/objects/function.h>
#include <src/objects/integer.h>
#include <src/objects/string.h>
#include <src/objectsystem.h>
#include <src/unicode.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include "utils.h"


void test_objects_simple(void)
{
	struct Object *objectclass = interpreter_getbuiltin(testinterp, NULL, "Object");
	buttert(objectclass);
	struct Object *obj = classobject_newinstance(testinterp, NULL, objectclass, (void *)0xdeadbeef);
	OBJECT_DECREF(testinterp, objectclass);
	buttert(obj);
	buttert(obj->data == (void *)0xdeadbeef);
	OBJECT_DECREF(testinterp, obj);
}

void test_objects_error(void)
{
	struct Object *err = NULL;
	errorobject_setwithfmt(testinterp->builtinctx, &err, "oh %s", "shit");
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

// TODO: test actually running this thing to make sure that data is passed correctly
struct Object *callback(struct Context *callctx, struct Object **errptr, struct Object **args, size_t nargs)
{
	buttert(nargs == 2);
	buttert(args[0] == (flipped ? callback_arg2 : callback_arg1));
	buttert(args[1] == (flipped ? callback_arg1 : callback_arg2));
	return (struct Object*) 0x123abc;
}

void test_objects_function(void)
{
	buttert((callback_arg1 = stringobject_newfromcharptr(testinterp, NULL, "asd1")));
	buttert((callback_arg2 = stringobject_newfromcharptr(testinterp, NULL, "asd2")));

	struct Object *func = functionobject_new(testinterp, NULL, callback);
	buttert(functionobject_call(testinterp->builtinctx, NULL, func, callback_arg1, callback_arg2, NULL) == (struct Object*) 0x123abc);

	struct Object *partial1 = functionobject_newpartial(testinterp, NULL, func, callback_arg1);
	OBJECT_DECREF(testinterp, callback_arg1);   // partialfunc should hold a reference to this
	OBJECT_DECREF(testinterp, func);
	flipped = 0;
	buttert(functionobject_call(testinterp->builtinctx, NULL, partial1, callback_arg2, NULL) == (struct Object*) 0x123abc);

	struct Object *partial2 = functionobject_newpartial(testinterp, NULL, partial1, callback_arg2);
	OBJECT_DECREF(testinterp, callback_arg2);
	OBJECT_DECREF(testinterp, partial1);
	flipped = 1;    // arg 2 was partialled last, so it will go first
	buttert(functionobject_call(testinterp->builtinctx, NULL, partial2, NULL) == (struct Object*) 0x123abc);

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
	struct Object *ret = method_call(testinterp->builtinctx, NULL, s, "to_string", NULL);
	buttert(ret);
	buttert(ret == s);
	OBJECT_DECREF(testinterp, s);    // functionobject_call() returned a new reference
	OBJECT_DECREF(testinterp, s);    // stringobject_newfromustr() returned a new reference
}

void test_objects_string_newfromfmt(void)
{
	unicode_char bval = 'b';
	struct UnicodeString b;
	b.len = 1;
	b.val = &bval;

	struct Object *c = stringobject_newfromcharptr(testinterp, NULL, "c");
	buttert(c);

	struct Object *res = stringobject_newfromfmt(testinterp->builtinctx, NULL, "-%s-%U-%S-%D-%%-", "a", b, c, c);
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
		stringobject_newfromcharptr(testinterp, NULL, "a"),
		stringobject_newfromcharptr(testinterp, NULL, "b"),
		stringobject_newfromcharptr(testinterp, NULL, "c") };
#define NOBJS (sizeof(objs) / sizeof(objs[0]))
	for (size_t i=0; i < NOBJS; i++)
		buttert(objs[i]);

	struct Object *arr = arrayobject_new(testinterp, NULL, objs, NOBJS - 1);
	buttert(arr);
	buttert(arrayobject_push(testinterp, NULL, arr, objs[NOBJS-1]) == STATUS_OK);

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
	for (size_t i=0; i < HOW_MANY; i++) {
		// TODO: add integerobject_fromlonglong or something
		char buf[10];
		sprintf(buf, "%d", (int) (i+10));
		buttert((objs[i] = integerobject_newfromcharptr(testinterp, NULL, buf)));
	}

	struct Object *arr = arrayobject_new(testinterp, NULL, objs, HOW_MANY);
	check(arr->data);

	for (int i = HOW_MANY-1; i >= 0; i--) {
		struct Object *obj = arrayobject_pop(testinterp, arr);
		buttert(obj == objs[i]);
	}

	for (size_t i=0; i < HOW_MANY; i++)
		buttert(arrayobject_push(testinterp, NULL, arr, objs[i]) == STATUS_OK);
	check(arr->data);

	OBJECT_DECREF(testinterp, arr);
	for (size_t i=0; i < HOW_MANY; i++)
		OBJECT_DECREF(testinterp, objs[i]);
}
#undef HOW_MANY

void test_objects_integer(void)
{
	struct UnicodeString u;
	u.len = 5;
	u.val = bmalloc(sizeof(unicode_char) * 5);
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
		buttert(integerobject_tolonglong(posints[i]) == 123);
		buttert(integerobject_tolonglong(negints[i]) == -123);
		OBJECT_DECREF(testinterp, posints[i]);
		OBJECT_DECREF(testinterp, negints[i]);
	}

	// "0" and "-0" should be treated equally
	struct Object *zero1 = integerobject_newfromcharptr(testinterp, NULL, "0");
	struct Object *zero2 = integerobject_newfromcharptr(testinterp, NULL, "-0");
	buttert(integerobject_tolonglong(zero1) == 0);
	buttert(integerobject_tolonglong(zero2) == 0);
	OBJECT_DECREF(testinterp, zero1);
	OBJECT_DECREF(testinterp, zero2);

	// biggest and smallest allowed values
	// TODO: implement and test error handling for too big or small values
	struct Object *big = integerobject_newfromcharptr(testinterp, NULL, "9223372036854775807");
	struct Object *small = integerobject_newfromcharptr(testinterp, NULL, "-9223372036854775808");
	buttert(integerobject_tolonglong(big) == INT64_MAX);
	buttert(integerobject_tolonglong(small) == INT64_MIN);
	OBJECT_DECREF(testinterp, big);
	OBJECT_DECREF(testinterp, small);
}

// classobject isn't tested here because it's used a lot when setting up testinterp
