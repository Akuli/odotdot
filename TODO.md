# TODO list

This list contains things that are horribly broken, things that annoy me and
things that I would like to do some day. It's a mess.

## Language design changes
- infixes: ``(a `f` b)`` is equivalent to ``(f a b)``, ``a `f` b;`` is
  equivalent to ``f a b;``
    - later operators could be implemented as syntactic sugar for infixes, e.g.
      ``a + b`` might be ``a `(import "<stdlib>/operators").add` b``
    - `a + b*c` would be invalid syntax, would need to be written `(a + (b*c))`
        - good because people won't be confused by `a + b * c`, which means
          evaluating `b * c` first in math and many other programming languages
            - real mathematicians (tm) write `a + bc` to avoid this problem,
              needing parentheses would be ö's solution
- error handling so that tests can be written in ö
    - `throw "asd";` and ``{ ... } `catch` { ... };`` would be good enough for
      now
- attributes should not be just items of a mapping
    - problems with current setup:
        - some objects are special because they have no attributes
        - it's possible to add arbitrary attributes to any object that has
          attributes, so a typo could cause code not working (like in python)
        - it's not possible to implement something for customizing setting and
          getting attributes (python's `@property`)
        - built-in things would produce weird error messages if attributes were
          deleted unexpectedly
        - compare to python: it's *possible* but very confusing and bad style
          to add attributes outside `__init__`, shouldn't be possible in the
          first place IMO
            - there is `__slots__`, but nobody uses it
    - attributes should be represented as setter,getter pairs of the class, and
      all instances of the class should have the same attributes
- keyword arguments?
    - there are many many more important things to be done before these... but
      these might be useful
    - `var` is a keyword because `var a=123;` looks much better than
      `var "a" 123;`, but with keyword arguments, `var a=123;` could be
      implemented as calling a `var` function with a keyword argument
        - ambiguous? `a = 123;` still wouldn't be a function call
    - now there's no nice way to pass named options to functions
        - python solution: keyword arguments

            ```python
            def show_text(text, foreground='black', background='white'):
                ...

            # black text on white
            show_text('asd')

            # red text on blue
            show_text('asd', foreground='red', background='blue')
            ```

            nice and simple

        - javascript solution: passing a mapping object

            ```js
            function showText(text, options) {
                options = options || {};
                options.foreground = options.foreground || 'black';
                options.background = options.background || 'white';
                ...
            }

            # black text on white
            showText('asd');

            # red text on blue
            showText('asd', { foreground: 'red', background: 'blue' });
            ```

            kinda boilerplaty

        - ö solution: passing a mapping object (looks really stupid)

            ```
            func "show_text" "options?" {
                ...something ugly...
            };

            # black text on white
            show_text "asd";

            # red text on blue
            show_text "asd" (new Mapping [ ["foreground" "red"] ["background" "blue"] ]);
            ```

            many many things featured here are not implemented yet... sorry

            calling the function is really ugly!

            another alternative, this one looks like java (ewww):

            ```
            var options = (new ShowTextOptions);
            options.foreground = "red";
            options.background = "blue";
            show_text "asd" options;
            ```

            this one is simple to use, but has its quirks and is harder to
            implement, otherwise nice:

            ```
            show_text { foreground = "red"; background = "blue"; };
            ```

            TODO: write more about problems with this approach

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
- delete some unused and stupid macros from the end of unicode.h
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
