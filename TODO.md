# TODO list

This list contains things that are horribly broken, things that annoy me and
things that I would like to do some day. It's a mess.

## main.c
- should evaluate `stdlib/builtins.ö` on startup, then `argv[1]` in a subscope
    - `stdlib/builtins.ö` doesn't exist yet, it should be created
    - `stdlib/fake_builtins.ö` can stay and be used by pythoninterp for now

## tokenizer.h
- tokenizing an empty file fails
    - `token_ize()` returns a linked list, and an empty linked list is
      represented as `NULL`, which is also used for errors
- error handling should be done with error objects
- rename the functions? `token_ize()` is consistent with a `Token` prefix, but
  `tokenize()` reads nicer

## common.h
- this file should be removed
- all uses of `STATUS_NOMEM` should be removed
- `<stdbool.h>` should be used instead of `STATUS_OK` and `STATUS_ERROR`

## unicode.c and unicode.h
- delete UnicodeString?
    - string objects are quite capable anyway
    - now errors.h creates a string object without a `stringobject_newblabla()`
      function because `stringobject_newblabla()` functions use errptr and
      nomemerr
        - maybe add `stringobject_newnoerrptr()` or something for creating
          the `not enough memory` message string?
- error objects should be used in `utf8_{en,de}code()` error handling
    - I think the errors are always ignored right now because the error
      handling is so difficult to use correctly
    - setting an error from a C string decodes as UTF-8, but that can't
      recurse because:
        - `nomemerr` is created before `utf8_{en,de}code()` are used, so if
          the encoder or decoder runs out of memory it will just use an
          existing exception
            - "not enough memory" is ASCII only, no need to use utf8
              functions when creating nomemerr
        - if the input string is invalid, the invalid parts are displayed
          in hexadecimal and decoding them can't fail
            - `errorobject_setwithfmt()` should probably support an
              equivalent of `%#X`, or maybe just add `errorobject_setsprintf()`
              or something for using all printf formatting features

## ast.c and ast.h
- add nonzero linenos to expression nodes
    - debugging would be a lot easier
- use an enum for ast node kinds?
    - more debugger-friendly (maybe? not sure if that's a big deal)
    - then again, values like `'1'` for numbers and `'.'` for attributes are
      quite debuggable
- error handling in ast.c sucks dick, should use error objects instead

## equals.h
this file sucks, must figure out a more customizable alternative

## gc.c and gc.h
- add some way to clean up reference cycles while the interpreter is running?
  maybe by invoking a library function after doing something unusually refcycly?
    - not sure if this will ever be needed, reference cycles should be rare-ish
      and nobody will notice the problem anyway
