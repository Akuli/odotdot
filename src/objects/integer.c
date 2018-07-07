// TODO: implement bignums? maybe a pure-ö BigInteger class?
// TODO: better error handling than asserts

#include "integer.h"
#include <assert.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>
#include "../check.h"
#include "../interpreter.h"
#include "../method.h"
#include "../objectsystem.h"
#include "../unicode.h"
#include "array.h"
#include "bool.h"
#include "classobject.h"
#include "errors.h"
#include "function.h"
#include "null.h"
#include "string.h"


// returns false on error
static bool parse_ustr(struct Interpreter *interp, struct UnicodeString ustr, long long *res)
{
	struct UnicodeString origstr = ustr;
	if (ustr.len == 0) {
		errorobject_throwfmt(interp, "ValueError", "'' is not an integer");
		return false;
	}

	bool isnegative=(ustr.val[0] == '-');
	if (isnegative) {
		ustr.val++;
		ustr.len--;
	}

	// remove leading zeros, but leave a single digit zero alone
	// e.g. 0000000123 --> 123, 00000000 --> 0, 000001 --> 1
	while (ustr.len > 1 && ustr.val[0] == '0') {
		ustr.val++;
		ustr.len--;
	}

	if (ustr.len == 0) {
		errorobject_throwfmt(interp, "ValueError", "'%U' is not an integer", origstr);
		return false;
	}
	if (ustr.len > INTEGEROBJECT_MAXDIGITS) {
		// FIXME: make sure to get the correct error message
		// here the number can be either way too large or contain characters not in 0-9
		goto mustBbetween;
	}

	int digits[INTEGEROBJECT_MAXDIGITS];
	for (int i=0; i < (int)ustr.len; i++) {
		if (ustr.val[i] < '0' || ustr.val[i] > '9') {
			errorobject_throwfmt(interp, "ValueError", "'%U' is not an integer", origstr);
			return false;
		}
		digits[i] = ustr.val[i] - '0';
	}

	// careful here... -INTEGEROBJECT_MIN overflows, but -(INTEGEROBJECT_MIN+1) doesn't
#define ABSMAX (isnegative ? ( (unsigned long long)(-(INTEGEROBJECT_MIN+1)) ) + 1 : (unsigned long long)INTEGEROBJECT_MAX)
	unsigned long long absval = 0;

	unsigned long long multipleby = 1;
	for (int i = ustr.len-1; i>=0; i--) {
		if (absval > ABSMAX - digits[i]*multipleby)
			goto mustBbetween;
		absval += digits[i]*multipleby;   // fitting was just checked
		multipleby *= 10;
	}
#undef ABSMAX

	*res = isnegative ? -((long long)(absval-1)) - 1 : (long long)absval;
	return true;

mustBbetween:
	errorobject_throwfmt(interp, "ValueError", "integers must be between %s and %s, but '%U' is not", INTEGEROBJECT_MINSTR, INTEGEROBJECT_MAXSTR, origstr);
	return false;
}

static void integer_destructor(void *data)
{
	free(data);
}

// (new Integer "123") converts a string to an integer
static struct Object *newinstance(struct Interpreter *interp, struct Object *args, struct Object *opts)
{
	if (!check_args(interp, args, interp->builtins.Class, interp->builtins.String, NULL)) return NULL;
	if (!check_no_opts(interp, opts)) return NULL;
	struct Object *string = ARRAYOBJECT_GET(args, 1);

	long long *data = malloc(sizeof(long long));
	if (!data) {
		errorobject_thrownomem(interp);
		return NULL;
	}
	if (!parse_ustr(interp, *((struct UnicodeString*) string->objdata.data), data)) {
		free(data);
		return NULL;
	}

	struct Object *integer = object_new_noerr(interp, ARRAYOBJECT_GET(args, 0), (struct ObjectData){.data=data, .foreachref=NULL, .destructor=integer_destructor});
	if (!integer) {
		free(data);
		return NULL;
	}

	integer->hash = (unsigned int) *data;
	return integer;
}

struct Object *integerobject_newfromlonglong(struct Interpreter *interp, long long val)
{
	assert(INTEGEROBJECT_MIN <= val && val <= INTEGEROBJECT_MAX);

	long long *data = malloc(sizeof(long long));
	if (!data) {
		errorobject_thrownomem(interp);
		return NULL;
	}
	*data = val;

	struct Object *integer = object_new_noerr(interp, interp->builtins.Integer, (struct ObjectData){.data=data, .foreachref=NULL, .destructor=integer_destructor});
	if (!integer) {
		errorobject_thrownomem(interp);
		free(data);
		return NULL;
	}
	integer->hash = (unsigned int) val;
	return integer;
}

struct Object *integerobject_newfromustr(struct Interpreter *interp, struct UnicodeString ustr)
{
	long long val;
	if (!parse_ustr(interp, ustr, &val))
		return NULL;
	return integerobject_newfromlonglong(interp, val);
}

long long integerobject_tolonglong(struct Object *integer)
{
	return *((long long *) integer->objdata.data);
}


// overrides Object's setup to allow arguments
// the arguments will go to newinstance
static struct Object *setup(struct Interpreter *interp, struct ObjectData thisdata, struct Object *args, struct Object *opts) {
	return nullobject_get(interp);
}

// TODO: implement in pure ö when division works
static struct Object *to_string(struct Interpreter *interp, struct ObjectData thisdata, struct Object *args, struct Object *opts)
{
	if (!check_args(interp, args, NULL)) return NULL;
	if (!check_no_opts(interp, opts)) return NULL;

	long long val = *((long long *) ((struct Object*) thisdata.data)->objdata.data);
	if (val == 0)   // special case
		return stringobject_newfromcharptr(interp, "0");

	assert(INTEGEROBJECT_MIN <= val && val <= INTEGEROBJECT_MAX);

	// see parse_ustr
	unsigned long long absval;   
	if (val < 0) {
		// '-' will be added to res later
		absval = ( (unsigned long long)(-(val+1)) ) + 1;
	} else {
		absval = val;
	}

	// res is filled in backwards because it's handy
	char res[INTEGEROBJECT_MAXDIGITS+1];
	res[INTEGEROBJECT_MAXDIGITS] = 0;
	int i = INTEGEROBJECT_MAXDIGITS;
	while( absval != 0) {
		res[--i] = (char)(absval % 10) + '0';
		absval /= 10;
	}
	if (val < 0)
		res[--i] = '-';

	return stringobject_newfromcharptr(interp, res+i);
}

struct Object *integerobject_createclass(struct Interpreter *interp)
{
	struct Object *klass = classobject_new(interp, "Integer", interp->builtins.Object, newinstance);
	if (!klass)
		return NULL;

	if (!method_add(interp, klass, "setup", setup)) goto error;
	if (!method_add(interp, klass, "to_string", to_string)) goto error;
	return klass;

error:
	OBJECT_DECREF(interp, klass);
	return NULL;
}


// TODO: these are a lot of boilerplate
static struct Object *eq(struct Interpreter *interp, struct ObjectData nulldata, struct Object *args, struct Object *opts)
{
	if (!check_args(interp, args, interp->builtins.Object, interp->builtins.Object, NULL)) return NULL;
	if (!check_no_opts(interp, opts)) return NULL;
	struct Object *x = ARRAYOBJECT_GET(args, 0), *y = ARRAYOBJECT_GET(args, 1);
	if (!(classobject_isinstanceof(x, interp->builtins.Integer) && classobject_isinstanceof(x, interp->builtins.Integer)))
		return nullobject_get(interp);
	return boolobject_get(interp, integerobject_tolonglong(x)==integerobject_tolonglong(y));
}

static struct Object *add(struct Interpreter *interp, struct ObjectData nulldata, struct Object *args, struct Object *opts)
{
	if (!check_args(interp, args, interp->builtins.Object, interp->builtins.Object, NULL)) return NULL;
	if (!check_no_opts(interp, opts)) return NULL;
	struct Object *x = ARRAYOBJECT_GET(args, 0), *y = ARRAYOBJECT_GET(args, 1);
	if (classobject_isinstanceof(x, interp->builtins.Integer) && classobject_isinstanceof(x, interp->builtins.Integer))
		return integerobject_newfromlonglong(interp, integerobject_tolonglong(x) + integerobject_tolonglong(y));
	return nullobject_get(interp);
}

static struct Object *sub(struct Interpreter *interp, struct ObjectData nulldata, struct Object *args, struct Object *opts)
{
	if (!check_args(interp, args, interp->builtins.Object, interp->builtins.Object, NULL)) return NULL;
	if (!check_no_opts(interp, opts)) return NULL;
	struct Object *x = ARRAYOBJECT_GET(args, 0), *y = ARRAYOBJECT_GET(args, 1);
	if (classobject_isinstanceof(x, interp->builtins.Integer) && classobject_isinstanceof(x, interp->builtins.Integer))
		return integerobject_newfromlonglong(interp, integerobject_tolonglong(x) - integerobject_tolonglong(y));
	return nullobject_get(interp);
}

static struct Object *mul(struct Interpreter *interp, struct ObjectData nulldata, struct Object *args, struct Object *opts)
{
	if (!check_args(interp, args, interp->builtins.Object, interp->builtins.Object, NULL)) return NULL;
	if (!check_no_opts(interp, opts)) return NULL;
	struct Object *x = ARRAYOBJECT_GET(args, 0), *y = ARRAYOBJECT_GET(args, 1);
	if (classobject_isinstanceof(x, interp->builtins.Integer) && classobject_isinstanceof(y, interp->builtins.Integer))
		return integerobject_newfromlonglong(interp, integerobject_tolonglong(x) * integerobject_tolonglong(y));
	return nullobject_get(interp);
}

// TODO: div

static struct Object *lt(struct Interpreter *interp, struct ObjectData nulldata, struct Object *args, struct Object *opts)
{
	if (!check_args(interp, args, interp->builtins.Object, interp->builtins.Object, NULL)) return NULL;
	if (!check_no_opts(interp, opts)) return NULL;
	struct Object *x = ARRAYOBJECT_GET(args, 0), *y = ARRAYOBJECT_GET(args, 1);
	if (classobject_isinstanceof(x, interp->builtins.Integer) && classobject_isinstanceof(y, interp->builtins.Integer))
		return boolobject_get(interp, integerobject_tolonglong(x) < integerobject_tolonglong(y));
	return nullobject_get(interp);
}


bool integerobject_initoparrays(struct Interpreter *interp)
{
	if (!functionobject_add2array(interp, interp->oparrays.eq, "integer_eq", eq)) return false;
	if (!functionobject_add2array(interp, interp->oparrays.add, "integer_add", add)) return false;
	if (!functionobject_add2array(interp, interp->oparrays.sub, "integer_sub", sub)) return false;
	if (!functionobject_add2array(interp, interp->oparrays.mul, "integer_mul", mul)) return false;
	if (!functionobject_add2array(interp, interp->oparrays.lt, "integer_lt", lt)) return false;
	return true;
}
