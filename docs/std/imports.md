# imports

`<std>/imports` contains things that are useful for doing magic with the
import function. See [the importing documentation](../importing.md) if you just
want to use libraries the usual way.

## Importers

`Library` is a simple class that inherits from
[ArbitraryAttribs](../builtins.md#arbitraryattribs). `new Library` takes no
arguments or options, and `Library` has no methods. You can access the
`Library` class e.g. like this:

```python3
var Library = (get_class (import "<std>/imports"));
```

An importer is a function that is called with two arguments, a string passed to
`import` and the [stack frame] that `import` was called from. It should return
a `Library` object or `null`. `imports.importers` is an [array] of importers.

Here's an example, let's call it `importer.ö`:

```python3
import "<std>/imports" as: "imports";
var Library = (get_class imports);

func "lol_importer string stackframe" {
    if (string == "<std>/lol") {
        var lib = (new Library);
        lib.wat = "woot";
        return lib;
    };
    return null;
};

imports.importers.push lol_importer;
```

Add this `testie.ö` file to the same directory with `importer.ö`:

```python3
import "importer";    # must be before the <std>/lol import
import "<std>/lol" as: "lol";
debug lol.wat;       # prints "woot"
```

Customizing the behaviour of `import` is that simple. There's no `lol.ö` file
anywhere; `(import "<std>/lol")` just called the `lol_importer`, and it
returned `lib`. If, however, there was a `lol.ö` in Ö's [std](../../std)
directory, it would be loaded instead. Importers are called in order, and the
importer that loads `.ö` files was added to `importers` before `lol_importer`
was added.

Note that our `lol_importer` is not [cached] in any way; it's called again
every time `<std>/lol` is imported. You can make a cached importer e.g.
like this:

```python3
var cache = (new Mapping);

func "cached_importer string stackframe" {
    catch {
        return (cache.get string);
    } KeyError { };

    if (string == "something") {
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
