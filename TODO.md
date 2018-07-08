# TODO list

This list contains things that are horribly broken, things that annoy me and
things that I would like to do some day. It's a mess.

## Language design stuff

- `class`: document `getter` and `setter`

    example usage:

    ```python
    class "Lol" {
        getter "x" { return 1; };
        setter "x" "new_x" { debug new_x; };
    };

    debug (new Lol).x;
    (new Lol).x = 123;
    ```

- `class`: document `abstract`

    example usage:

    ```python
    class "Lol" {
        abstract "x";
    };

    # now (new Lol).x throws AttribError
    ```

- `for`: want break and continue
    - they would also work in `while` right away because `while` is implemented
      like this:

        ```
        func "while condition body" {
            for {} condition {} body;
        };
        ```

- oopy List baseclass and stuff to `std/datastructures.ö` or something,
  or maybe `std/lists.ö`?
    - similar stuff for Mapping

    - maybe it'd be best to rename Mapping to HashTable, and have
      Mapping be an abstract class

- repl: handle multiline input
    - the parser should return a different value for unexpected end-of-file
      than other errors
    - when an unexpected EOF occurs, just read another line with a `...` prompt
      or something and concatenate

- string formatting

    this is not very good:

    ```
    var lol = ((((a + ", ") + b) + " and ") + c);
    ```

    is hard to debug and maintain and write and everything and i hate it

    i want something like

    ```
    var lol = "${a}, ${b} and ${c}";
    var wut = "it costs \$100";   # escaping the $ for a literal dollar sign
    ```

    which could be implemented with an `eval` function


## testing
- there are lots of ötests already... which is good
- need to go through the docs and test every case documented
    - also look through the source and try to find funny corner cases
- ask friends for help? writing tests is a lot of work
    - although not as much work as i expected, ö is quite nice to work with :)

## tokenizer.{c,h}
- tokenizing an empty file fails
    - `token_ize()` returns a linked list, and an empty linked list is
      represented as `NULL`, which is also used for errors
- error handling should be done with error objects
    - especially annoying in the repl
- rename the functions? `token_ize()` is consistent with a `Token` prefix, but
  `tokenize()` reads nicer

## ast.{c,h}
- use an enum for ast node kinds?
    - more debugger-friendly (maybe? not sure if that's a big deal)
    - then again, values like `'1'` for numbers and `'.'` for attributes are
      quite debuggable

## parse.{c,h}
- error handling in ast.c sucks dick, should use error objects instead
    - especially annoying in the repl
- make ast nodes possible to inspect, manipulate and create from ö code?

## gc.{c,h}
- add some way to clean up reference cycles while the interpreter is running?
  maybe by invoking a library function after doing something unusually refcycly?
    - not sure if this will ever be needed, reference cycles should be rare-ish
      and nobody will notice the problem anyway
- find and use an actual garbage collector instead of reinventing the wheel?

## more problems!

see [builtins documentation](docs/builtins.md) and ctrl+f for e.g. "annoyances"
or "missing features"

also `grep -r 'TODO\|FIXME' src` if you dare!
