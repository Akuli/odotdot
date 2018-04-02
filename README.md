# Ö

Ö is a programming language with a weird-ish syntax.

    <Akuli>      i like how 'ding = (dang dong)' translates to 'ding = dang(dong)',
                 just move the ( to the right side of the function name
    <Zaab1t>     haha
    <Zaab1t>     syntax is odd yeah

## Hello World!

Make sure you have Python 3.4 or newer and git installed. Then download the
interpreter:

    $ git clone https://github.com/akuli/odotdot ö
    $ cd ö

Save this to `hello.ö`:

```python
var msg = "hello world";
print msg;
```

Here `print` is a function, not a keyword. Functions are just called without
parentheses.

Run the code like this:

    $ python3 -m pythoninterp hello.ö

My favorite feature with this language is blocks. Anything between `{`
and `}` is a block, and blocks have a `.run` method that works like
this:

```python
var helloblock = {
    print "hello world";
};

# run it 3 times
helloblock.run;
helloblock.run;
helloblock.run;
```

Of course, every block gets a new scope; variables defined in `{ }` are
not visible outside of the `{ }`:

```python
{
    var a = "hello";
    print a;    # prints hello
}.run;
print a;    # error!
```

See [the examples directory](examples/) for more!

## FAQ

### How do I type ö?

With the ö key. It's right next to the ä key.

### Why did you name the programming language Ö?

Ö is used quite often in Finnish, and people who aren't used to it think that
it looks like a face with mouth fully opened.

    <Zaab1t>       looks like someone scared
    <Zaab1t>       ö

See also David Beazley's [Meẗal](https://github.com/dabeaz/me-al).

### Why is the repository named odotdot instead of ö?

Because github.

### The interpreter is implemented in Python. Isn't a module named ö against PEP-8?

`ö` is a perfectly PEP-8 compatible module name. [PEP-8
says](https://www.python.org/dev/peps/pep-0008/#ascii-compatibility) that
identifiers should be ASCII only, but module names don't need to be identifiers:

```python
import ö as odotdot     # now the identifier is named odotdot
```

You can also just `import ö` because the ASCII limitation in PEP-8 is only for
Python's standard library.

## Tests

At the time of writing this README, I have horrible test coverage. I'll
probably write more tests when the language supports catching exceptions and I
can write some of the tests in the language itself.

If you want to see for yourself how horrible the coverage is, run these
commands:

    $ python3 -m coverage run -m pytest
    $ python3 -m coverage html
    $ yourfavoritebrowser htmlcov/index.html
