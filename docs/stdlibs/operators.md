# operators

`<stdlibs>/operators` lets you customize the behaviour of things like
`(a == b)` and `(a + b)`.


## Operator arrays

An operator call like `(x + y)` makes the interpreter loop through an array of
functions. It calls each function with `x` and `y` as arguments, and if the
function returns something else than `null`, that's the result of `(x + y)` and
no more functions will be called. If all functions in the array return `null`,
a [TypeError] is thrown.

The array used for `(x + y)` is accessible as
`(import "<stdlibs>/operators").add_array`. This function behaves as if it was
defined as `func "do_plus x y" { return (x+y); };`:

```python
var operators = (import "<stdlibs>/operators");

func "do_plus x y" {
    foreach "function" operators.add_array {
        var result = (function x y);
        if (not (result `same_object` null)) {
            return result;
        };
    };

    throw (new TypeError "oh no");
};
```

This means that you can overload operators by pushing your own functions to the
`add_array`. [lambda] and [is_instance_of] are useful here. Here's an example:

```python
var operators = (import "<stdlibs>/operators");

class "Lol" {
    attrib "thing";
    method "setup thing" {
        this.thing = thing;
    };
};

# (lol1 + lol2) returns a lol with things added
operators.add_array.push (lambda "lol1 lol2" {
    if ((lol1 `is_instance_of` Lol) `and` (lol2 `is_instance_of` Lol)) {
        return (new Lol (lol1.thing + lol2.thing));
    };
    return null;
});

var lola = (new Lol "a");
var lolb = (new Lol "b");
print (lola + lolb).thing;    # prints "ab"
```

Usually it's best to use [is_instance_of] in the functions added to `add_array`
as shown above, but you don't need to do that; you can check the values in any
way you want. However, note that the function should silently return `null` for
any values that it doesn't care about.

`<stdlibs>/operators` contains more arrays for customizing operators:

| This...       | ...loops through this array and calls the functions as described above    |
| ------------- | ------------------------------------------------------------------------- |
| `(a + b)`     | `add_array`                                                               |
| `(a - b)`     | `sub_array`                                                               |
| `(a * b)`     | `mul_array`                                                               |
| `(a / b)`     | `div_array`                                                               |
| `(a == b)`    | `eq_array`                                                                |
| `(a < b)`     | `lt_array`                                                                |

There's no type checking for `==` and `<`; it's possible to override `==` to
return a `Lol` object (although that wouldn't work with `!=` because [not]
needs a [Bool], see below).

All other operators are implemented using the operators described above:

| This...       | ...actually does this         | Notes                                                 |
| ------------- | ----------------------------- | ----------------------------------------------------- |
| `(a != b)`    | `(not (a == b))`              |                                                       |
| `(a > b)`     | `(b < a)`                     | `(a > b)` evaluates `a` before `b`, unlike `(b < a)`  |
| `(a <= b)`    | ``((a < b) `or` (a == b))``   | `a` and `b` are evaluated only once, `a` first        |
| `(a >= b)`    | ``((a > b) `or` (a == b))``   | `a` and `b` are evaluated only once, `a` first        |

The `and` and `or` functions are documented [here][and] and [here][or].

In this documentation, "`(a != b)` actually does `(not (a == b))`" means that
`(a != b)` behaves like `(not (a == b))`; that is, it returns `true` if
`(a == b)` returns `false`, or `false` if `(a == b)` returns `true`, and throws
[TypeError] otherwise. However, if someone replaces the built-in `not` function
with their own `not`, it is undefined how the interpreter behaves then; doing
that may or may not change what `(a != b)` does.


[examples/operator_overload.ö]: ../../examples/operator_overload.ö
[TypeError]: ../errors.md
[lambda]: ../builtins.md#lambda
[is_instance_of]: ../builtins.md#is_instance_of
[or]: ../builtins.md#or
[not]: ../builtins.md#not
[Bool]: ../builtins.md#Bool
