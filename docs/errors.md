# Errors

Like most other things, errors are represented with objects in Ö. For example,
if you try to print an [Integer](builtins.md#integer)...

```python
print 123;
```

...you get something like `TypeError: expected an instance of String, got 123`.

You can also display a similar error by creating an instance of the `TypeError`
class and calling the `throw` function; for example, this code produces a
similar error:

```python
throw (new TypeError "expected an instance of String, got 123");
```

All of the classes in the following list are in the [built-in scope], and their
[setup methods] take 1 argument, the error message string.

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
  arguments or an unsupported option. However, if the arguments are of the
  wrong type, `TypeError` should be used instead.
- `AssertError` is thrown by [assert](builtins.md#assert).
- `AttribError` is thrown when an attribute is not found. For example,
  `[].aasdasd` and `[].aasdasd = "lol";` throw `AttribError`.
- `KeyError` is thrown when a key of a mapping is not found.
  `(new Mapping [[1 2] [3 4]]).get 5;` throws a `KeyError`.
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


## Catching Errors

This sucks and will hopefully be fixed soon. See the documentation of
[the catch function](builtins.md#catch).


## Custom Errors

Creating new errors is easy. Just inherit from `Error`, like this:

```python
class "ArticleNotFoundError" inherits:Error { };

func "get_article name" {
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
[setup methods]: tutorial.md#defining-classes
