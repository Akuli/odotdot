# TODO list

This list contains things that are horribly broken, things that annoy me and
things that I would like to do some day. It's a mess.

## Language design stuff
- mappings should *not* be hashable, this is a bug in the interpreter
    - also check what `==` does with mappings, should compare their content
- importing? here is what i have in mind right now:
    - `(import "<stdlib>/x")` loads `stdlib/x.ö`, or something like
      `stdlib/x/setup.ö` if `stdlib/x.ö` doesn't exist and `stdlib/x` is a
      directory
        - maybe warn if there is an `x` directory AND an `x.ö` file?
    - `"<stdlib>"` is special, other paths are treated as relative to the
      importing file
    - ideally importing would be implemented in pure Ö, but many other things
      must be done before that:
        - file I/O
        - path utilities: checking if the path is absolute, `\\` on windows etc
            - speaking of windows... i'd like to learn a less unixy alternative
              to make and try to compile ö on windows some day
        - expose a function that converts a long string to an AST tree
        - how to make the path relative to the calling file? see stack
        - see the attribute stuff below
- operators
    - syntactic sugar for infixes? e.g. ``a + b`` might be
      ``a `(import "<stdlib>/operators").add` b``
    - `a + b*c` would be invalid syntax, would need to be written `(a + (b*c))`
        - good because people won't be confused by `a + b * c`, which means
          evaluating `b * c` first in math and many other programming languages
        - real mathematicians (tm) write `a + bc` to avoid this problem, but
          `ab` mean `a` times `b` in ö so parentheses would be ö's solution
- error handling issues
    - the `throw` function isn't actually implemented yet, but calling it
      throws an exception anyway because nonexisting variables cause errors :D
    - `catch` can't catch the error object yet, so tests can't check exception
      messages :( can't really write good tests in Ö yet
- need tracebacks and other stack magic
    - might be easiest to get the traceback in `Error.new`? that way
      rethrowing would be easy as `throw caught_error;`
    - maybe traceback printing could be implemented in pure ö?
    - c functions that push and pop to an array of stack info objects
    - maybe expose the stack info in ö?
    - stack infos should contain at least file name (absolute path?) and lineno
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
        - how about modules? might be nice to have a python-style module system
          where functions defined in modules are exposed as attributes of a
          module object
        - maybe add a flag that allows setting arbitrary attributes?

            ```
            class Module {
                arbitrary_attributes = true;
                ...
            };

            var toot = (new Module);
            toot.asd = "lol wat";
            ```

        - or maybe a special class that does some magic?

            ```
            class Module inherits ArbitraryAttributes {
                ...
            };
            ```

- keyword arguments?
    - there are many many more important things to be done before these... but
      these might be useful
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

        - java solution: making a config object

            ```java
            ShowTextOptions options = new ShowTextOptions();
            options.setForeground("red");
            options.setBackground("blue");
            showText("asd", options);
            ```

            lots of boilerplate everywhere!

        - possible, bad and shit ö solutions
            - javascript way, looks ugly

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

            - java way, also ugly

                ```
                var options = (new ShowTextOptions);
                options.foreground = "red";
                options.background = "blue";
                show_text "asd" options;
                ```

            - code block magic

                ```
                show_text { foreground = "red"; background = "blue"; };
                ```

                simple to use, has its quirks and is hard to implement

                TODO: write more about problems with this approach

            - new syntax ftw :D

                ```
                func "show_text" "foreground:" "background:" {
                    ...
                };

                show_text "asd";
                show_text "asd" foreground:"red" background:"blue";
                ```

                constructing mappings with string keys could be easy as in
                javascript, these would be equivalent:

                ```
                thing = (new Mapping [["a" 1] ["b" 2] ["c" 3]]);
                thing = (new Mapping a:1 b:2 c:3);
                ```

            - could be also done with `=` instead of `:`, like in python

                ```
                show_text "asd" foreground="red" background="blue";
                ```

                maybe `var` wouldn't need to be a keyword?

                ```
                var some_var="hi";   # this could be just a function call
                ```

                but this would cause very confusing errors:

                ```
                some_function arg1 arg2 blah blah    # oops! missing ;
                some_var = "lol";           # now this is a named argument to some_function
                ```

    - would be good for avoiding stupid marker objects

        ```
        # without keyword arguments
        class "Thing" inherits Thingy {    # inherits would be a marker object created in builtins.ö
            ...
        };

        # with keyword arguments
        class "Thing" inherits:Thingy {
            ...
        };

        # with keyword arguments, different coding style
        class "Thing"
            inherits: Thingy
        {
            ...
        };
        ```

## docs
- the builtins need to be document well, now `Array` methods are only listed in
  the tutorial and `Mapping` methods are missing
- `syntax-spec.md` is quite complete, need tests that check every possible
  detail of it

## tokenizer.{c,h}
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

## unicode.{c,h}
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

## ast.{c,h}
- add nonzero linenos to expression nodes
    - debugging would be a lot easier
- use an enum for ast node kinds?
    - more debugger-friendly (maybe? not sure if that's a big deal)
    - then again, values like `'1'` for numbers and `'.'` for attributes are
      quite debuggable
- error handling in ast.c sucks dick, should use error objects instead

## equals.{c,h}
these suck, must figure out a more customizable alternative

if dispatchable functions will exist some day, maybe like this:

```
# this stuff would go to a special file, e.g. stdlib/operators.ö

dispatchable_func "equals a b";
equals.dispatch [Integer Integer] {
    return (some magic that computes a+b);
};
equals.dispatch [String String] {
    return (some magic that concatenates the strings);
};

# now ("a" == "b") would call the string thing and (1 == 2) would call the integer thing
```

## gc.{c,h}
- add some way to clean up reference cycles while the interpreter is running?
  maybe by invoking a library function after doing something unusually refcycly?
    - not sure if this will ever be needed, reference cycles should be rare-ish
      and nobody will notice the problem anyway
