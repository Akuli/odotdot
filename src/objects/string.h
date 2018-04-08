#ifndef OBJECTS_STRING_H
#define OBJECTS_STRING_H

#include "../interpreter.h"
#include "../objectsystem.h"
#include "../unicode.h"

// this takes the Object classinfo instead of the interpreter and returns NULL for no mem
// because this function is called before error objects like interp->nomemerr exists
struct ObjectClassInfo *stringobject_createclass(struct ObjectClassInfo *objectclass);

// should be called when other stuff is set up, see builtins_setup()
// returns STATUS_OK or STATUS_ERROR
int stringobject_addmethods(struct Interpreter *interp, struct Object **errptr);

// makes a copy of ustr with unicodestring_copy(), returns NULL on error
// RETURNS A NEW REFERENCE
struct Object *stringobject_newfromustr(struct Interpreter *interp, struct Object **errptr, struct UnicodeString ustr);

// asserts that the char pointer is valid utf8, i.e. don't use with user-inputted strings
// RETURNS A NEW REFERENCE
struct Object *stringobject_newfromcharptr(struct Interpreter *interp, struct Object **errptr, char *ptr);

// the returned UnicodeString's val must NOT be free()'d
struct UnicodeString *stringobject_getustr(struct Object *str);


#endif   // OBJECTS_STRING_H
