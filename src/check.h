#ifndef CHECK_H
#define CHECK_H

struct Interpreter;
struct Object;

// checks if obj is an instance of klass, returns STATUS_OK or STATUS_ERROR
// bad things happen if klass is not a class object
int check_type(struct Interpreter *interp, struct Object *klass, struct Object *obj);

/* for example, do this in the beginning of a functionobject_cfunc...

        if (check_args(interp, argarr, interp->builtins.stringclass, interp->builtins.integerclass, interp->builtins.objectclass, NULL) == STATUS_ERROR)
                return NULL;

...to make sure that:
        * the function is being called with 3 arguments
        * the first argument is a String
        * the second argument is an Integer
        * the 3rd argument can be anything because all classes inherit from Object

bad things happen if the ... arguments are not class objects or you forget the NULL
returns STATUS_OK or STATUS_ERROR
*/
int check_args(struct Interpreter *interp, struct Object *argarr, ...);

#endif   // CHECK_H
