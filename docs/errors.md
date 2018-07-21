# Errors

Like most other things, errors are represented with objects in Ö. For example,
if you try to print an [Integer](builtins.md#integer)...

```python
print 123;
```

...you get something like `TypeError: expected an instance of String, got 123`.

All of the error classes in the following list are in the [built-in scope], and
their [setup methods] take 1 argument, the error message string.

- `Error` is a base class for other error classes. Don't create `Error` objects
  directly like this:

  ```python
  throw (new Error "the value is too small");
  ```

  Use one of the classes documented below instead, e.g. like this:

  ```python
  throw (new ValueError "the value is too small");
  ```
- `ArgError` is thrown when a function is called with the wrong number of
  arguments or an unsupported optional argument. However, if the arguments are
  of the wrong type, `TypeError` should be used instead.
- `AssertError` is thrown by [assert](builtins.md#assert).
- `AttribError` is thrown when an attribute is not found. For example,
  `[].aasdasd` and `[].aasdasd = "lol";` throw `AttribError`.
- `IoError` is thrown when [an IO operation](std/io.ö) fails. Files are read
  when importing, so [import] can also throw this error in some cases.
- `KeyError` is thrown when a key of a mapping is not found.
  `(new Mapping [[1 2] [3 4]]).get 5;` throws a `KeyError`.
- `MarkerError` is used internally by `return`. There's usually no need to
  catch it, and even if you catch a `MarkerError`, there's no documented way to
  check which return value it represents or which function call it came from.
- `MathError` is currently never thrown by any built-in functions as addition
  is the only supported math operation (I know, it sucks). In the future, this
  will probably be used for things like division by zero.
- `MemError` is thrown when the system runs out of memory. Pretty much any
  function can throw it, but these are rare.
- `TypeError` is raised when the type of an object is incorrect. See the
  example above.
- `ValueError` is raised when some object has an invalid or incorrect value.
  For example, `(new Integer "hello")` throws `ValueError` because `"hello"` is
  not a valid integer value.
- `VariableError` is thrown when a variable is not found. Usually it's best to
  avoid getting this error instead of catching it.

Attributes of `Error`:
- `error.message` is the message as a human-readable string. This can be set
  after creating the error object, but setting it to something else than a
  [String](builtins.md#string) throws `TypeError`.
- `error.stack` is an [Array] of StackFrame objects, empty if the error has
  never been thrown. See [the stacks library's documentation](std/stacks.md)
  for more information about stack frames. When throwing the error, the content
  of `error.stack` is set only if the stack is non-empty; see
  [rethrowing](#rethrowing). The `stack` itself cannot be set to a new array,
  but the objects in the `stack` may be modified.

Methods of `Error`:
- `error.print_stack;` prints a stack trace. If `error.stack` is empty
  (usually it means that the error has never been thrown), this prints
  `SomeError: message` where `message` is `error.message` and `SomeError` is
  the name of the error's class. If the stack is non-empty, the
  `SomeError: message` part is followed by details about which lines of code
  caused the error.
- `error.(to_debug_string)` returns a string like `<SomeError: "message">`
  where `SomeError` and `message` mean the same things as above. See also
  [Object](builtins.md#object)'s `to_debug_string` documentation.

Note that all of the above classes have the attributes and methods that `Error`
has because they are subclasses of `Error`.


## Throwing

You can display errors by creating an instance of an error class and calling
the `throw` function. For example, this code produces an error similar to the
one we got from `print 123;`:

```python
throw (new TypeError "expected an instance of String, got 123");
```

The [built-in] `throw` function takes exactly 1 argument, the error, and no
optional arguments.


## Catching

The [built-in] `catch` function is called like
`catch block1 errorclass block2;`, where `block1` and `block2` are
[Block](builtins.ö#block) objects and `errorclass` is a class that inherits
from `Error`, e.g. one of the classes above.

`catch` tries to run `block1`, and if doing that throws an error that matches
`errorclass`, `block2` runs as well. For example:

```python
catch {
    print 123;
} TypeError {
    print "cannot print Integers";
};
```

If you want to catch the error instance in a variable, pass a
`[errorclass varname]` [array](builtins.md#array) instead of `errorclass`:

```python
catch {
    print 123;
} [TypeError "e"] {
    debug e;      # prints <TypeError: "expected an instance of String, got 123">
};
```

The matching is checked with [is_instance_of](builtins.md#is_instance_of), and
the blocks are ran in new subscopes of their
[definition scopes][definition scope]. The variable named by `varname` (`e` in
the above example) is set as a local variable to the scope that `block2` runs
in before running `block2`.

If `block1` raises an error that does *not* match `errorclass`, the `catch`
call doesn't do anything to that error; that is, the error gets handled as if
`catch` wasn't used at all in the first place, and `block2` doesn't run. If
`block2` throws *any* error, `catch` doesn't do anything to that either.

If the `errorclass` or `[errorclass varname]` pair is invalid, `catch` throws
an error without running either of the blocks. If the `[errorclass varname]`
array contains the wrong number of elements, a `ValueError` is thrown;
otherwise the thrown error is a `TypeError`.


## Rethrowing

You can throw an error as many times as you want, like this:

```python
catch {
    print 123;
} [TypeError "e"] {
    print "it failed";
    throw e;    # this is known as rethrowing
};
```

When the `print 123;` part fails, `"it failed"` is printed, but otherwise
everything behaves just as if `print 123;` was called without `catch`. Note
that rethrowing is not shown in the stack traces; if you run the above program,
the line that `throw e;` is on won't be shown in the error message.


## Custom Errors

Creating new errors is easy. Just inherit from `Error`, like this:

```python
class "ArticleNotFoundError" inherits:Error { };

func "get_article article_name" {
    ...

    if (not found_article) {
        throw (new ArticleNotFoundError ("there's no article named ".concat article_name));
    };
    ...
};
```

It's best to create and use custom error classes sparingly. A good guideline is
to only create custom errors when it's important to be able to catch those
errors. For example, if you would instead `throw (new ValueError ...)`,
`get_article` calls would need to be surrounded by code that catches
`ValueError`. Catching `ArticleNotFoundError` looks nice because it's easy to
see what the code is doing, but it's also good because if `get_article`
unexpectedly throws `ValueError` somewhere else, that won't get caught.


[built-in scope]: tutorial.md#scopes
[built-in]: tutorial.md#scopes
[definition scope]: tutorial.md#scopes
[setup methods]: tutorial.md#defining-classes
[import]: builtins.md#import
