# TODO list

This list contains things that are horribly broken, things that annoy me and
things that I would like to do some day. It's a mess.

## Language design stuff
- operators
    - syntactic sugar for infixes? e.g. ``a + b`` might be
      ``a `(import "<stdlibs>/operators").add` b``
    - `a + b*c` would be invalid syntax, would need to be written `(a + (b*c))`
        - good because people won't be confused by `a + b * c`, which means
          evaluating `b * c` first in math and many other programming languages
        - real mathematicians (tm) write `a + bc` to avoid this problem, but
          `ab` doesn't mean `a` times `b` in ö so parentheses would be ö's
          solution
- StackInfo objects should contain the scope
    - local variables are more debuggable?
        - but i'm not planning on implementing a debugger any time soon
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

- `for`: want break and continue
    - they would also work in `while` right away because `while` is implemented
      like this:

        ```
        func "while condition body" {
            for {} condition {} body;
        };
        ```

- need `switch` and `case`

    this is the only thing that can be done now:

    ```
    if (x `equals` 1) {
        print "a";
    } else: {
        if (x `equals` 2) {
            print "b";
        } else: {
            if (x `equals` 3) {
                print "c";
            } else: {
                print "d";
            };
        };
    };
    ```

    a `Mapping` could help but it feels like a workaround

    ```
    var lol = (new Mapping [[1 "a"] [2 "b"] [3 "c"]]);
    print (lol.get_with_default x "d");
    ```

    i want something like:

    ```
    switch x {
        case 1 { print "a"; };
        case 2 { print "b"; };
        case 3 { print "c"; };
        default { print "d"; };
    };
    ```

    it can be implemented in pure ö

- string formatting

    this sucks:

    ```
    var lol = ((((a.concat ", ").concat b).concat " and ").concat c);
    ```

    and is hard to debug and maintain and write and everything and i hate it

    i want something like

    ```
    var lol = "${a}, ${b} and ${c}";
    var wut = "it costs \$100";   # escaping the $ for a literal dollar sign
    ```


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
- rename the functions? `token_ize()` is consistent with a `Token` prefix, but
  `tokenize()` reads nicer

## ast.{c,h}
- use an enum for ast node kinds?
    - more debugger-friendly (maybe? not sure if that's a big deal)
    - then again, values like `'1'` for numbers and `'.'` for attributes are
      quite debuggable
- error handling in ast.c sucks dick, should use error objects instead
- make ast nodes possible to inspect, manipulate and create from ö code?

## equals.{c,h}
these suck, must figure out a more customizable alternative

if dispatchable functions will exist some day, maybe like this:

```
# this stuff would go to a special file, e.g. stdlibs/operators.ö

dispatchable_func "equals a b";
equals.dispatch [Integer Integer] {
    return something;
};
equals.dispatch [String String] {
    return something;
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
