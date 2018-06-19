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
[an examples directory](examples/) and [more documentation](docs/).

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

Some of the tests are written in C and some tests are written in Ö.
`runtests` is a shell script that runs all tests:

    $ ./runtests

See `./runtests --help` for more options.

I'm not using a coverage tool because I don't know how to use any C coverage
tools, and there are much more important things to fix than bad coverage; see
[my huge TODO list](TODO.md).
