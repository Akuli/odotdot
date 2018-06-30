#ifndef CHECK_H
#define CHECK_H

#include <stdbool.h>

struct Interpreter;
struct Object;

// checks if obj is an instance of klass, returns false on error
// bad things happen if klass is not a class object
bool check_type(struct Interpreter *interp, struct Object *klass, struct Object *obj);

/* for example, do this in the beginning of a functionobject_cfunc...

	if (!check_args(interp, args, interp->builtins.String, interp->builtins.Integer, interp->builtins.Object, NULL)) return NULL;
	if (!check_no_opts(interp, opts)) return NULL;

...to make sure that:
	* the function is being called with 3 arguments and no options
	* the first argument is a String
	* the second argument is an Integer
	* the 3rd argument can be anything because all classes inherit from Object

bad things happen if the ... arguments are not class objects or you forget the NULL
returns false on error
*/
bool check_args(struct Interpreter *interp, struct Object *args, ...);

// check_args calls this
// types must be an Array object that contains classes, bad things happen otherwise
bool check_args_with_array(struct Interpreter *interp, struct Object *args, struct Object *types);

/* this...

	if (!check_opts(interp, opts, messagestr, interp->builtins.String, idstr, interp->builtins.Integer, NULL))
		return NULL;

...where messagestr is stringobject_newfromcharptr(interp, "message") and idstr similarly, makes sure that:
	* the function was called with other options than "message" or "id"
	* id is an Integer if it was given
	* message is a String if it was given

btw, consider adding string constants to interp->strings
*/
bool check_opts(struct Interpreter *interp, struct Object *opts, ...);

// if you don't need options
#define check_no_opts(interp, opts) check_opts((interp), (opts), NULL)

/* check_opts calls this

bad things happen if:
	* opts and types are not Mappings
	* keys of opts or types are not Strings
	* values of types are not classes
*/
bool check_opts_with_mapping(struct Interpreter *interp, struct Object *opts, struct Object *types);

#endif   // CHECK_H
