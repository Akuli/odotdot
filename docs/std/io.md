# io

`<std>/io` contains things for doing IO (Input and Output). Usually this
library is used for reading and writing files (that is, "file IO"), but it
contains base classes and some other IO stuff as well.

## Example

This code writes "Hellö Wörld!" followed by a newline character to `hello.txt`:

```python
var io = (import "<std>/io");

io.(open "hello.txt" writing:true).as "file" {
    file.write "Hellö Wörld!\n";
};
```

Now you can open `hello.txt` in your favorite editor, and you should see the
hellö wörld. You can also read the file in Ö:

```python
io.(open "hello.txt" reading:true).as "file" {
    debug file.(read_all);     # prints "Hellö Wörld!\n"
};
```

In these examples, we're writing [strings][String] to the file. However, files
are not limited to just storing strings, and they actually contain
[bytes][ByteArray] instead. The `file` is a [StringWrapper](#stringwrapper)
object that converts [String]s to [ByteArray]s and [ByteArray]s to [String]s
automatically.

File objects must be closed when they are no longer being used. The `as` method
creates a new [subscope] of the block's [definition scope], adds the file
object there as a local variable and runs the block. However, it *always*
closes the file. This code is **bad**:

```python
var file = io.(open "hello.txt" reading:true);
debug file.(read_all);
file.close;
```

If `file.(read_all)` [throws an error][errors], `file.close;` is never called.
If the `as` method is used instead, the file is *always* closed.


## FileLike

This is a base class for all objects that should behave like file objects that
work with [ByteArray]s. File objects that work with strings are actually
[StringWrapper](#stringwrapper) objects that wrap a `FileLike`. For
example, [open](#open) with `binary:true` returns an instance of a subclass of
`FileLike`. However, without `binary:true` or with `binary:false`,
[open](#open) returns a [StringWrapper](#stringwrapper).

In the rest of this `FileLike` documentation, "a file" means "a file-like
object". I'm calling them files because I'm lazy, but these things work with
any other file-like object as well.

Accessing *any* attribute or calling *any* method listed below should raise a
[ValueError] if `.closed` is `true`. Keep this in mind if you implement your
own file-like object by inheriting from this class.

File objects keep track of a *position* that is set to 0 initially. Reading *N*
bytes from the file means getting a slice of the content of length *N* starting
at the *position*'th byte, and incrementing the position by *N*.

Attributes:
- `file_like.closed` is `true` or `false` depending on whether
  `file_like.close;` has been called. This must be overrided in a subclass.

Methods:
- `file_like.close;` should be always called after using the file. Calling
  `file_like.close;` should flush the file implicitly so that
  `file_like.flush; file_like.close;` behaves just like `file_like.close;`.
- `file_like.check_closed;` throws [ValueError] if the file is closed. This is
  useful for implementing methods and attributes in subclasses.
- `file_like.(read_chunk maxsize)` reads `maxsize` or less bytes, and returns
  them as a [ByteArray]. If there are less than `maxsize` bytes left before the
  end of file, the file is read to the end and the bytes before the end are
  returned. This means that reading a file whose position is already at the end
  returns an empty [ByteArray].
- `file_like.(read_all)` calls `read_chunk` repeatedly until the file is read
  to the end, and returns the results as a [ByteArray].
- `file_like.write bytearray;` saves the bytes from a [ByteArray] to the file
  or prepares them to be actually saved when `flush` is called.
- `file_like.flush;` makes sure that the data is actually written to the file
  so that it's visible when e.g. reading the file with another file object.
- `file_like.(get_pos)` returns the position of the file as an [Integer]. The
  position is never negative.
- `file_like.set_pos pos;` sets the position to `pos`. If `pos` is negative, a
  `ValueError` is raised, but setting the position to a value too big isn't
  guaranteed to do anything useful *or* raise an error. Don't do it.
- `file_like.as varname block` creates a new [subscope] of `block`'s
  [definition scope], sets a local variable named `varname` to `file_like`, and
  runs `block` in the subscope. If `block` doesn't throw an error,
  `file_like.close;` is called. If `block` throws an error, `file_like.close;`
  is called anyway and the error is [rethrown][rethrowing].

Not all files support reading *and* writing, so one of `read` and `write` may
always throw `ValueError` to indicate that the file can be used only for
writing or reading. Similarly, `set_pos` and `get_pos` don't need to be
supported, and they always throw `ValueError` unless they are overridden.

If you have hard time remembering the name of the `flush` method, think of
file objects as toilets. If you use a toilet, some of the stuff may or may not
end up going down the pipes, but everything will go down when you *flush* the
toilet.


## FakeFile

Use this class if someone has written a function for working with file objects,
but you want to use it with data that doesn't come from a file. Here is an
example:

```python
var io = (import "<std>/io");

func "print_content file" {
    print file.(read_all);
};

var fake_file = (new io.StringWrapper (new io.FakeFile));
fake_file.write "hello";
fake_file.wrapped.set_pos 0;
print_content fake_file;    # prints hello
```

If you want to work with [ByteArray]s instead of [String]s, don't use a
[StringWrapper](#stringwrapper). Note that unlike with most other file-like
objects, there's no need to close `FakeFile`s; however, `close` is supported
for compatibility with functions that need to close the files.

`FakeFile` inherits [FileLike](#filelike), overrides all
[FileLike](#filelike) methods that must be overrided and doesn't have
any methods that [FileLike](#filelike) doesn't have.


## StringWrapper

This is *not* a subclass of [FileLike](#filelike) because
`FileLike` objects are for working with bytes. A `StringWrapper` created
like `(new StringWrapper filelike)` has similar attributes and methods as the
file-like object, but it converts [String]s to [ByteArray]s and [ByteArray]s to
[String]s for you.

Attributes:
- `stringwrapper.wrapped` is the [FileLike](#filelike) object passed to
  `new StringWrapper`.
- `stringwrapper.encoding` is the [encoding][encodings] used when converting
  between [String] and [ByteArray]. This can be set when creating the
  `stringwrapper` like `(new StringWrapper filelike encoding:encoding_name)`
  where `encoding_name` is a [String] (see [encodings]). The default is
  `"utf-8"`.
- `stringwrapper.closed` is equivalent to `stringwrapper.wrapped.closed`.

Methods:
- `stringwrapper.close;` calls `stringwrapper.wrapped.close;`.
- `stringwrapper.flush;` calls `stringwrapper.wrapped.flush;`.
- `stringwrapper.as varname block;` is like [FileLike](#filelike) `as` method, but
  this is *not* the same as `stringwrapper.wrapped.as varname block;` because
  that sets the variable to `stringwrapper.wrapped`, but `stringwrapper.as`
  sets it to the `stringwrapper`.
- `stringwrapper.(read_all)` calls `stringwrapper.wrapped.(read_all)` and
  converts the result to a [String].
- `stringwrapper.write string;` converts the string to a [ByteArray] and
  calls `stringwrapper.wrapped.write`.
- `stringwrapper.(read_line)` reads the file until it finds a `\n` character
  or the file ends, and returns an [Option] of the line without `\n`. If the
  file is already at the end and nothing can be read, [none] is returned
  instead.

Note that there's no `read_chunk` method. If you want to read chunks of data,
it probably makes sense to work with [ByteArray]s instead of [String]s and you
shouldn't use [StringWrapper].


## open

`(open path reading:true)` and `(open path writing:true)` return
[StringWrapper](#stringwrapper)s of file objects for reading and writing files.
If `path` is not an absolute path, the file is opened in the directory that the
Ö interpreter was invoked from. Here is a list of options that `open` takes:

- `reading` ([Bool], default is `false`): if this is `true`, the file can be
  read with e.g. `read_all`.
- `writing` ([Bool], default is `false`): if this is `true`, data can be
  written to the file. If the file exists already, the old content is deleted.
- `binary` ([Bool], default is `false`): if this is `false`, everything is set
  up in a way that makes the returned object useful for working with strings.
  This means that:
    - The resulting file is wrapped with a [StringWrapper](#stringwrapper).
    - On platforms that don't represent newlines with the `\n` character,
      newlines are converted automatically so that your Ö code will work
      correctly if it uses `"\n"`.
    - The `encoding` argument can be used (see below).
  None of that happens with `binary:true`, which is useful for working with
  other files than text files, e.g. pictures or music.
- `encoding` ([String], see [encodings], default is `"utf-8"`): this is passed
  to `StringWrapper` with `binary:false`, and using this with `binary:true`
  throws [ArgError].

Note that at least one of `reading` and `writing` must be set to `true`;
otherwise, an [ArgError] is thrown. If they are both `true`, the file appears
to be empty when reading because `writing:true` emptied it.


[ByteArray]: ../builtins.md#bytearray
[String]: ../builtins.md#string
[Integer]: ../builtins.md#integer
[Bool]: ../builtins.md#bool
[is_instance_of]: ../builtins.md#is_instance_of
[subscope]: ../tutorial.md#scopes
[definition scope]: ../tutorial.md#scopes
[errors]: ../errors.md
[rethrowing]: ../errors.md#rethrowing
[encodings]: encodings.md
[ArgError]: ../errors.md
[ValueError]: ../errors.md
[Option]: ../builtins.md#option
