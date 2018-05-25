# Syntax Specification

This page defines the `ö` syntax that the interpreter can execute.

## Expressions

An expression is any piece of code that returns a value. For example:

```python
print "hello";
```

Here `"hello"` is an expression that returns a `String` object, and `print` is
an expression that returns the value of the `print` variable.

Here are examples of all supported kinds of expressions:

- String literals: `""` and `"hello"` return `String` objects.
- Integer literals: `123`, `-5` and `0` return `Integer` objects.
- Variable lookups: `print` and `magic_number` return values of variables.
- Attribute lookups: `expr.attr` returns the `.attr` attribute of the object
  returned by the `expr` expression.
- Method lookups: `expr::meth` returns the `::meth` method of the object
  returned by the `expr` expression.
- List literals: `[element1 element2 element3]` and `[]` return `List` objects.
  The elements can be any expressions.
- Function call expressions: `(function arg1 arg2 arg3)` calls the function
  with the given arguments and returns whatever the function returned. The
  function and the arguments can be any expressions, and you can have any
  number of arguments you want.
- Infixed function call expressions: ``(arg1 `function` arg2)`` is equivalent
  to `(function arg1 arg2)`. Here `func`, `arg1` and `arg2` can be any
  expressions.
- Blocks: `{ statement1; statement2; }` returns a block object. You can have
  any number of [statements](#statements) inside the block you want. `{ }` is a
  code block that does nothing, and `{ expression }` without a `;` is
  equivalent to `{ return expression; }`.

So far we have called functions like `function a b c;` instead of
`(function a b c)`. The big difference is that the function's return value is
ignored with a statement like `function a b c;`, but `(function a b c)` is an
expression that returns that value instead of ignoring it.

## Statements

A statement is anything that ends with a `;`, like `print "hello";`.

Here's another list:

- Function calls: `function arg1 arg2 arg3;` is like the
  `(function arg1 arg2 arg3)` [expression](#expressions), but the return value
  is ignored.
- Infixed function calls: ``arg1 `function` arg2;`` is equivalent to
  `function arg1 arg2;`.
- Variable creation: `var a = b;` sets the variable `a` to `b` **locally**.
  `a` can be any variable name, and `b` can be any expressions.
- Assignments: `a = b;` is like `var a = b;`, but it sets the variable
  **wherever it's defined**. If the variable hasn't been defined anywhere,
  this throws an error.
- Setting attributes: `a.b = c;` sets the `b` attribute of `a` to `c`. `b` can
  be any attribute name, and `a` and `c` can be any expressions.

The global and local variable stuff is probably kind of confusing. Here's an
example that should make it clear:

```python
var a = "old a";

{
    print a;            # old a
    var a = "new a";    # set it locally
    print a;            # new a
}::run;
print a;    # old a

{
    print a;                # old a
    a = "really new a";     # set it where it was defined
    print a;                # really new a
}::run;
print a;    # really new a
```
