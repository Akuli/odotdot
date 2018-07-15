# TODO list

This list contains things that are horribly broken, things that annoy me and
things that I would like to do some day. It's a mess.

## class: docs are outdated

- document getter and setter

    example usage:

    ```python
    class "Lol" {
        getter "x" { return 1; };
        setter "x" "new_x" { debug new_x; };
    };

    debug (new Lol).x;
    (new Lol).x = 123;
    ```

- document `abstract`

    example usage:

    ```python
    class "Lol" {
        abstract "x";
    };

    # now (new Lol).x throws AttribError
    ```

## for: want break and continue

they would also work in `while` right away because `while` is implemented
like this:

    ```
    func "while condition body" {
        for {} condition {} body;
    };
    ```

## Mapping and Option: get_with_default should probably be replaced with an option

```
var x = (option.get);                 # throws an error if option is null
var y = (option.get fallback:"asd");  # doesn't throw errors
```

## oopy List baseclass and stuff

- maybe this should go to e.g. `std/datastructures.ö` or `std/lists.ö`
- similar stuff for Mapping
- maybe it'd be best to rename Mapping to HashTable, and have
  Mapping be an abstract class

## repl: handle multiline input
- the parser should return a different value for unexpected end-of-file
  than other errors
- when an unexpected EOF occurs, just read another line with a `...` prompt
  or something and concatenate

## document repl

now it's just mentioned in the readme

## string formatting

this is not very good:

```
var lol = ((((a + ", ") + b) + " and ") + c);
```

is hard to debug and maintain and write and everything and i hate it

i want something like

```
var lol = "${a}, ${b} and ${c}";
var wut = "it costs \$100";   # escaping the $ for a literal dollar sign
```

this could be implemented in pure ö if an `eval` function is added:

```
var lol = (format "${a}, ${b} and ${c}");
```

`format` is too long though, should be `fmt` or something

if the non-function-call alternative is chosen, a function is needed anyway
because things like gettext work so that `"cannot open ${filename}"` is
translated to a non-english language (e.g. finnish:
`"tiedostoa ${filename} ei voi avata"`), and the value of `filename` must
be substituted **after** the translation

so it would look like this:

```
var message = (format (translate "cannot open ${filename}"));
```

## eval and compile_ast
- would be useful for testing and the pure-ö string formatting alternative
- for testing, we also need `compile_ast` or something that takes a string
  and returns an array of ast nodes
    - `eval` could just call `compile_ast`, and then create a `Block`
      object and run it
    - if there was this and an io lib, `file_importer` could be pure ö

## ByteArray objects

not documented yet! here are old notes about what these are:
- would be like `Array`s of `Integer`s between 0 and 255, but represented
  in C as arrays of unsigned char, taking up a lot less space
- useful for e.g. io without implicit encoding and decoding

# Ö functions: utf8_encode, utf8_decode, chr

not documented yet!

maybe should add a small `<std>/encodings` library with support for adding more
encodings

## i/o lib
- can't be implemented in pure ö unless i get a magic cffi thing working
- stack traces would include source lines etc... good stuffs

plan so far:
- `open` factory function (pure Ö)
    - filepath would be the only positional argument, other arguments follow
    - reading,writing (both default to false, exactly one of these must be true)
    - binary (defaults to false): used for several things:
        - windows CRLF to LF
        - windows file kind thingy
        - default encoding (see below)
    - encoding
        - the resulting file is wrapped with a `TextReader` or `TextWriter` unless `binary:true` is used and `encoding` is not given
        - `binary:false` means that the default is `"utf-8"`
        - if `binary:false` needs to be used without TextReader or TextWriter, must open with an encoding and access `.raw`
    - more options could be added later:
        - seek_to_end: like `a` in c
        - overwrite_existing: see python's `x` mode
- Reader and Writer (pure Ö)
    - nice base classes for implementing custom file-likes, these work with ByteArrays
    - Reader methods: read_chunk, read_all, seek
    - Writer methods: write, flush
    - both classes have these methods: close, close_after_running
    - these methods must be overrided in subclasses: read_chunk, write, close
- FileReader and FileWriter (implemented in C)
    - inherit Reader and Writer respectively
        - harder because Reader and Writer are pure ö
        - just need to make .baseclass attribute settable so these things' baseclass can be set to Reader and Writer in the ö code
    - these only come from open, no need to export
    - these classes are not responsible for implicit encoding and decoding
    - both take a positional `filepath` argument
    - common options from open: binary, seek_to_end, overwrite_existing
- TextReader and TextWriter (pure ö)
    - public constructor takes Reader or Writer as only argument
    - `.byte_stream` attribute is the Reader or
    - TextReader methods: read_line, read_all
        - no seek because it's broken in languages like python
            - ö is 2 bytes, but seek wants bytes
            - well-done seeking would require knowing how many bytes everything before the seek position is
        - read_chunk will probably be hard to implement for similar reasons that i don't feel like explaining now
    - TextWriter methods: write, flush
    - both TextAsder classes have these methods: close, close_after_running
- TODOs
    - TextReader read_chunk
    - read-and-write files could be implemented with multiple inheritance or something else bizarre

## recursive `to_debug_string`s?

```
ö> var a = [];
ö> a.push a;
ö> debug a;
...and we have a huge error message...
```

python does this:

```
>>> a = []
>>> a.append(a)
>>> a
[[...]]
>>>
```

maybe should use the `…` character instead?

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
    - especially annoying in the repl
- rename the functions? `token_ize()` is consistent with a `Token` prefix, but
  `tokenize()` reads nicer

## ast.{c,h}
- use an enum for ast node kinds?
    - more debugger-friendly (maybe? not sure if that's a big deal)
    - then again, values like `'1'` for numbers and `'.'` for attributes are
      quite debuggable

## parse.{c,h}
- error handling in ast.c sucks dick, should use error objects instead
    - especially annoying in the repl
- make ast nodes possible to inspect, manipulate and create from ö code?

## gc.{c,h}
- add some way to clean up reference cycles while the interpreter is running?
  maybe by invoking a library function after doing something unusually refcycly?
    - not sure if this will ever be needed, reference cycles should be rare-ish
      and nobody will notice the problem anyway
- find and use an actual garbage collector instead of reinventing the wheel?

## more problems!

see [builtins documentation](docs/builtins.md) and ctrl+f for e.g. "annoyances"
or "missing features"

also `grep -r 'TODO\|FIXME' src` if you dare!
