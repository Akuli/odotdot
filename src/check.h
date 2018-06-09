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

bool check_no_opts(struct Interpreter *interp, struct Object *opts);

#endif   // CHECK_H
