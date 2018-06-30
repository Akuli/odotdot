# TODO list

This list contains things that are horribly broken, things that annoy me and
things that I would like to do some day. It's a mess.

## Language design stuff

- `class`: support attributes with custom getters and setters
    - maybe something like this inside the `{ }`:

        ```
        attrib "_thingy_value";

        attrib "thingy" {
            # get and set would be functions
            get {
                return this._thingy_value;
            };
            set "new_value" {
                if (new_value `sucks` too_much) {
                    throw (new ValueError "it sucks!");
                };
                this._thingy_value = new_value
            };
        };
        ```

        many indents

    - or maybe this:

        ```
        attrib "_thingy_value";

        getter "thingy" {
            return this._thingy_value;
        };
        setter "thingy new_value" {
            if (new_value `sucks` too_much) {
                throw (new ValueError "it sucks!");
            };
            this._thingy_value = new_value;
        };
        ```

        straight-forward to implement, makes people new to ö think wtf are
        getter and setter doing

        then again, getter and setter are words that are also used in other
        programming languages, e.g. python

    - how about keyword arguments?

        ```
        attrib "_thingy_value";

        attrib "thingy"
        get: {
            return this._thingy_value;
        }
        set: {     # how to pass variable name "new_value" here??
            this._thingy_value = new_value;
        };
        ```

        several problems:
        - `;` goes to an unintuitive place, looks like it's missing
        - no nice way to pass the variable name

        `;` problem can be fixed with different coding style:

        ```
        attrib "thingy" get: {
            ...
        } set: {
            ...
        };
        ```

    in all cases, boilerplate getters can be simplified with the `{ asd }`
    means `{ return asd; }` syntax:

    ```
    attrib "thingy" {
        get { this._thingy_value };
        ...
    };
    ```

- `for`: want break and continue
    - they would also work in `while` right away because `while` is implemented
      like this:

        ```
        func "while condition body" {
            for {} condition {} body;
        };
        ```

- interactive repl? would be awesome!
    - multiline input could be a problem, would need to implement the parser so
      that it returns a different value for end-of-file than other errors
        - maybe (ab)use the EOF constant from i think `<stdlib.h>`? :D

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

## std/builtins.ö

this shouldn't be in std imo, i have no idea what happens if you
`import "<std>/builtins";`

probably something horrible

## tokenizer.{c,h}
- tokenizing an empty file fails
    - `token_ize()` returns a linked list, and an empty linked list is
      represented as `NULL`, which is also used for errors
- error handling should be done with error objects
- rename the functions? `token_ize()` is consistent with a `Token` prefix, but
  `tokenize()` reads nicer

## ast.{c,h}
- use an enum for ast node kinds?
    - more debugger-friendly (maybe? not sure if that's a big deal)
    - then again, values like `'1'` for numbers and `'.'` for attributes are
      quite debuggable

## parse.{c,h}
- error handling in ast.c sucks dick, should use error objects instead
- make ast nodes possible to inspect, manipulate and create from ö code?

## gc.{c,h}
- add some way to clean up reference cycles while the interpreter is running?
  maybe by invoking a library function after doing something unusually refcycly?
    - not sure if this will ever be needed, reference cycles should be rare-ish
      and nobody will notice the problem anyway

## more problems!

see [builtins documentation](docs/builtins.md) and ctrl+f for e.g. "annoyances"
or "missing features"

also `grep -r 'TODO\|FIXME' src` if you dare!
