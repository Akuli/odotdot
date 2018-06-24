# Importing

Ö has an `import` function that can be used for loading other files from an Ö
file. If you have a large project, it makes sense to put your code in multiple
files, and `import` lets those files use each other.

Let's start with a simple example. Here's `hello.ö`:

```python
func "say_hello" {
    print "Hellö Wörld!";
};
```

Here's `run.ö`.

```python
var hello = (import "hello");
debug hello;       # prints <Library at 0xblablabla>
hello.say_hello;   # prints "Hellö Wörld!"
```

`import` takes one string argument and returns a `Library` object. The
`Library` class is not in the [built-in scope], but you can access it easily
with `(get_class (import "something"))` if you need it.

The string that `import` takes as an argument is treated as a path to a file
relative to the location of the file that imports, and without the `.ö` part.
For example, if `blah/blah/run.ö` calls `(import "asd/toot")`, a file named
`blah/blah/asd/toot.ö` will be loaded. Windows paths use `\` instead of `/`,
e.g. `C:\Users\Someone` instead of `/home/someone`, but `import` replaces `/`'s
in the string with `\`'s on Windows, so you don't need to worry about it.

Common path tricks are supported; for example, running `(import "../thingy.ö")`
in `asd/toot/wat.ö` imports `asd/thingy.ö` because `..` means the parent
directory of `asd/toot`, which is `asd`.

If the string passed to `import` contains `<stdlibs>`, it will be replaced with
a path to Ö's [stdlibs](../stdlibs) directory. There's nothing useful there yet,
so unfortunately I can't provide an example of importing something from the
stdlibs.


## Caching

If you have a `hello.ö` that does something when it's imported, e.g. this...

```python
print "hello.ö was imported";
```

...and you have a `run.ö` like this...

```python
var hello1 = (import "hello");
var hello2 = (import "hello");
var hello3 = (import "hello");
```

...the `print` in `hello.ö` will **not** run 3 times; it'll run once only. This
means that if your library file is big and slow to load, that's not a problem;
it'll be loaded only once, even if you have many other files that need the
library file.


[built-in scope]: tutorial.md#scopes
