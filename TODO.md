# TODO list

This list contains things that are horribly broken, things that annoy me and
things that I would like to do some day. It's a mess.

## Language design stuff
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
          `ab` doesn't mean `a` times `b` in ö so parentheses would be ö's
          solution
- StackInfo objects should contain the scope
    - local variables are more debuggable?
        - but i'm not planning on implementing a debugger any time soon
    - if `import` is too hard to implement, i could implement a temporary
      `include` that runs the file in the caller's scope, this could be used
      for that
- there should be some way to have objects with arbitrary attributes, without
  needing to create a new type for each object
    - this would mean adding extra checks to pretty much every piece of c code
      that does something with attributes, fortunately attribute.h abstracts it
      away quite nicely
    - i want this because modules would be represented nicely as objects with
      exported symbols as attributes
        - maybe add a flag that allows setting arbitrary attributes?

            ```
            class "Module" {
                arbitrary_attributes = true;
                ...
            };

            var toot = (new Module);
            toot.asd = "lol wat";
            ```

        - or maybe a special class that does some magic?

            ```
            class "Module" inherits ArbitraryAttributes {
                ...
            };
            ```

- `class`: support attributes with custom getters and setters
    - maybe something like this inside the `{ }`:

        ```
        attrib "_thingy_value";

        attrib "thingy" {
            # get and set would be functions
            get {
                return = this._thingy_value;
            };
            set "new_value" {
                if (new_value `sucks` too_much) {
                    throw "it sucks!";
                };
                this._custom_value = new_value
            };
        };
        ```

        many indents

    - or maybe this:

        ```
        attrib "_thingy_value";

        getter "thingy" {
            return = this._thingy_value;
        };
        setter "thingy new_value" {
            if (new_value `sucks` too_much) {
                throw "it sucks!";
            };
            this._thingy_value = new_value;
        };
        ```

        straight-forward to implement, makes people new to ö think wtf are
        getter and setter doing

    - how about keyword arguments?

        ```
        attrib "_thingy_value";

        attrib "thingy"
        get: {
            return = this._thingy_value;
        }
        set: {     # how to pass variable name "new_value" here??
            this._thingy_value = new_value;
        };
        ```

        `;` goes to an unintuitive place, looks like it's missing

        big problem: no nice way to pass the variable name

    boilerplate getters can be simplified with the `{ asd }` means
    `{ return = asd; }` syntax:

    ```
    attrib "thingy" {
        get { this._thingy_value };
    ```

- loop functions
    - right now these are implemented with recursion, and it sucks
    - break and continue should be implemented with exceptions, just like return
    - maybe should provide a built-in for:

        ```
        for { init; } { cond } { incr; } {
            ...
        };
        ```

        and everything could be implemented with it:

        ```
        func "while cond body" {
            for {} cond {} body;
        };
        ```

        this is probably nice and simple

    - or maybe a built-in `loop_forever` and everything else with that?
        - `loop_forever` would need to provide continue and break
            - funny corner cases with continue works with `for` loops:

                this...

                ```
                for { init; } { cond } { incr; } {
                    stuff;
                };
                ```

                ...is NOT the same as this:

                ```
                init;
                while { cond } {
                    stuff;
                    incr;
                };
                ```
                because `incr` must run after `stuff` continues


## testing
- some tests suck, they are commented out in `ctests/run.c`
    - they started sucking when they required running `stdlib/builtins.ö` but i
      couldn't figure out a good way to run it run without lots of copy/pasta
    - still haven't figured out
    - everything has gotten less and less tested since then...
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
- rename the functions? `token_ize()` is consistent with a `Token` prefix, but
  `tokenize()` reads nicer

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

but i'm thinking of *not* making customizable callables, so maybe something
like this:

```
var equals_dispatches = [];
equals_dispatches.push (lambda "x y" {
    if ((x `isinstance` String) `and` (y `isinstance` String)) {
        return (some magic that checks the equality);   # returns true or false
    };
    return null;
});

# then something that loops through the array and checks for not-nulls
```

## gc.{c,h}
- add some way to clean up reference cycles while the interpreter is running?
  maybe by invoking a library function after doing something unusually refcycly?
    - not sure if this will ever be needed, reference cycles should be rare-ish
      and nobody will notice the problem anyway

## more problems!

see [builtins documentation](builtins.md) and ctrl+f for e.g. "annoyances" or
"missing features"
