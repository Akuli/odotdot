# stacks

If you run a program like this...

```python
func "f" { print 123; };
func "g" { f; };
func "h" { g; };
h;
```

...you get a *stack trace* roughly like this one:

```
TypeError: expected an instance of String, got 123
  in file fgh.ö, line 1
  by file fgh.ö, line 2
  by file fgh.ö, line 3
  by file fgh.ö, line 4
```

This stack trace is created from the `stack` attribute of the error object. If
you wrap everything in `catch` and [debug](../builtins.md#debug) the `stack` of
the error...

```python
func "f" { print 123; };
func "g" { f; };
func "h" { g; };
catch {
    h;
} [TypeError "e"] {
    debug e.stack;
};
```

...you get something like this:

```
[<StackFrame: file fgh.ö, line 4> <StackFrame: file fgh.ö, line 5> <StackFrame:
file fgh.ö, line 3> <StackFrame: file fgh.ö, line 2> <StackFrame: file fgh.ö, li
ne 1>]
```

The `stacks` attribute is documented [here](../errors.md), and rest of this
page documents the `<stdlibs>/stacks` library. It contains things for working
with stack frames.


## get_stack

This function takes no arguments or options, and returns an
[array](../builtins.md) of `StackFrame` objects similar to the `stack`
attribute of error objects. For example, this function prints the name of the
file it was called from (see [Importing](../importing.md)):

```python
func "who_called_me" {
    var stack = (get_stack);

    # last frame on the stack represents this who_called_me function
    # so we want the one before that
    # the index of the last frame is stack.length-1, that's why stack.length-2
    var caller_frame = (stack.get (stack.length.plus -2));
    print caller_frame.filename;
};
```


## StackFrame

Stack frames are instances of this class, and this class is available in the
`<stdlibs>/stacks` library.

Stack frames have these read-only attributes:
- `stackframe.filename` is the file name displayed in the stack trace, as a
  [String](../builtins.md#string).
- `stackframe.lineno` is the line number in the stack trace, as an
  [Integer](../builtins.md#integer).
