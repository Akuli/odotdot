#ifndef OBJECTS_STRING_H
#define OBJECTS_STRING_H

#include <stdarg.h>
#include "../interpreter.h"    // IWYU pragma: keep
#include "../objectsystem.h"   // IWYU pragma: keep
#include "../unicode.h"        // IWYU pragma: keep

// RETURNS A NEW REFERENCE or NULL on no mem, see builtins_setup()
struct Object *stringobject_createclass(struct Interpreter *interp);

// should be called when other stuff is set up, see builtins_setup()
// returns STATUS_OK or STATUS_ERROR
int stringobject_addmethods(struct Interpreter *interp, struct Object **errptr);

// makes a copy of ustr with unicodestring_copy(), returns NULL on error
// RETURNS A NEW REFERENCE
struct Object *stringobject_newfromustr(struct Interpreter *interp, struct Object **errptr, struct UnicodeString ustr);

// asserts that the char pointer is valid utf8, i.e. don't use with user-inputted strings
// RETURNS A NEW REFERENCE
struct Object *stringobject_newfromcharptr(struct Interpreter *interp, struct Object **errptr, char *ptr);

/* create a new string kinda like printf

fmt must be valid UTF-8, and it can contain any of these format specifiers:

	             ,---- cast if needed
	             |
	             V
	SPECIFIER  EXACT TYPE             NOTES
	----------------------------------------------------------------------------------
	%s         char *                 must be valid utf8
	%U         struct UnicodeString
	%S         struct Object *        to_string will be called
	%D         struct Object *        to_debug_string will be called
	%p         void *                 prints the pointer in the output, e.g. 0x1b6baa0
	%L         long long
	%%         nothing                a % in the output

	[*] remember to add a cast if needed, no implicit conversion here

nothing else works, not even padding like %5s

these fixed limits are NOT CHECKED even with assert:
	* stuff between format specifiers can be at most 200 characters long
	* number of format specifiers + number of texts between them must be <= 20
*/
struct Object *stringobject_newfromfmt(struct Interpreter *interp, struct Object **errptr, char *fmt, ...);

// like newfromfmt, but vprintf style
struct Object *stringobject_newfromvfmt(struct Interpreter *interp, struct Object **errptr, char *fmt, va_list ap);

// the returned UnicodeString's val must NOT be free()'d
struct UnicodeString *stringobject_getustr(struct Object *str);


#endif   // OBJECTS_STRING_H
