# imports

`<std>/imports` contains things that are useful for doing magic with the
import function. See [the importing documentation](../importing.md) if you just
want to use libraries the usual way.

## Importers

`Library` is a simple class that inherits from
[ArbitraryAttribs](../builtins.md#arbitraryattribs). `new Library` takes no
arguments or options, and `Library` has no methods.

An importer is a function that is called with two arguments, a string passed to
`import` and the [stack frame] that `import` was called from. It should return
a `Library` object or `null`. `imports.importers` is an [array] of importers.

Here's an example, let's call it `importer.ö`:

```python3
var imports = (import "<std>/imports");

func "lol_importer string stackframe" {
    if (string `equals` "<std>/lol") {
        var lib = (new imports.Library);
        lib.wat = "woot";
        return lib;
    };
    return null;
};

imports.importers.push lol_importer;
```

Add this `testie.ö` file to the same directory with `importer.ö`:

```python3
var lol = (import "<std>/lol");
debug lol.wat;       # prints "woot"
```

Customizing the behaviour of `import` is that simple. There's no `lol.ö` file
anywhere; `(import "<std>/lol")` just called the `lol_importer`, and it
returned `lib`.

Note that our `lol_importer` is not [cached] in any way; it's called again
every time `<std>/lol` is imported. You can make a cached importer e.g.
like this:

```python3
var cache = (new Mapping);

func "cached_importer string stackframe" {
    catch {
        return (cache.get string);
    } KeyError { };

    if (string `equals` "something") {
        var lib = (new imports.Library);
        ...add stuff to the lib...
        cache.set string lib;
        return lib;
    };
    return null;
};
```

The `importers` array contains some important stuff by default. You can add
more importers to it, but don't delete the importers that are there already;
that would probably make the interpreter crash in some way because nothing gets
imported.


[array]: ../builtins.md#array
[stack frame]: ../errors.md#stackframe-objects
[cached]: ../importing.md#caching
