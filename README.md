# Ö

Ö is a programming language with a weird-ish syntax.

    <Akuli>      i like how 'ding = (dang dong)' translates to 'ding = dang(dong)',
                 just move the ( to the right side of the function name
    <Zaab1t>     haha
    <Zaab1t>     syntax is odd yeah

## Hello World!

Install `make`, a C compiler and `git`. For example, if you're using a
Debian-based Linux distribution like Ubuntu or Mint, run this command:

    $ sudo apt install gcc make git

Then download and compile the interpreter.

    $ git clone https://github.com/akuli/odotdot
    $ cd odotdot
    $ make ö

You should get lots of output, but no errors.

Save this code to `hello.ö`...

```js
var msg = "Hellö Wörld!";
print msg;
```

...and run it:

    $ ./ö hello.ö
    Hellö Wörld!

See [the tutorial](docs/tutorial.md) for more. There's also
[an examples directory](examples/), but at the time of updating this README,
most examples don't actually work yet. Some day they should work...

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

## Tests

At the time of writing this README, I have horrible test coverage. I'll
probably write more tests when the language supports catching exceptions and I
can write some of the tests in the language itself.

If you want to see for yourself how horrible the coverage is, run these
commands:

    $ python3 -m coverage run -m pytest
    $ python3 -m coverage html
    $ yourfavoritebrowser htmlcov/index.html
