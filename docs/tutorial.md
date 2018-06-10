# Ö Tutorial

In [the README](../README.md), we downloaded and compiled the latest Ö
interpreter, and we ran a classic Hello World program in it. Of course, you can
do a lot more in Ö, and in this tutorial we'll look into some of its awesome
features.

**Everything in this tutorial should work with the latest Ö interpreter.** If
it doesn't work, please [let me know](https://github.com/Akuli/odotdot/issues/new).


## Powerful Syntax

Here's some Ö code.

```python
var doit = true;
if doit {
    print "hello";
};
```

The code doesn't look particularly interesting, but the `if` thing is actually
a function call! **`if` is just a function**, and functions are always called
without parentheses, like `print "hello";` instead of `print("hello");`. Here
`{ print "hello"; }` is an argument to the `if` function, and this program is
equivalent to the above program:

```python
var doit = true;
var lol = {
    print "hello";
};

# now lol is a Block object, let's pass it to the if function
if doit lol;
```

There are no commas between arguments, so Ö has `some_function a b` instead of
`some_function(a, b)`. Note that you must end `if` calls with `};` instead of
`}`, just like you need to do `print "asd";` instead of `print "asd"`:

```python
if something {
    ...
}  # <--- THIS IS WRONG... be careful
```

In fact, Ö's function call syntax is so powerful that **var is the only keyword**
in Ö. Most things that are done with keywords in most other languages, like
loops, functions and classes, are implemented with functions in Ö.


## Debugging

There's no Ö debugger yet, and even in languages that have debuggers, most
programmers don't bother with learning to use debuggers because learning that
is painful. Instead, people print stuff. If you want to figure out what the
value of a variable is, you might be tempted to do this:

```python
...lots of code...
print some_buggy_variable;
...more code...
```

**This is bad** because Ö's [print](builtins.md#print) can only print strings,
so if `some_buggy_variable` is set to something else than a string, e.g. an
integer or an array, the `print` will fail. Do this instead:

```python
...lots of code...
debug some_buggy_variable;
...more code...
```

This will print the value of the buggy variable in a programmer-readable form.
For example, `debug "lol";` prints `"lol"` including the quotes, and
`debug [1 2 3];` prints `[1 2 3]`. The `[1 2 3]` is an array; there are more
details about arrays [below](#arrays).


## Function Call Expressions

So far we have called functions so that we throw away the return value, like
this:

```python
some_function a b;
```

This is like `some_function(a, b);` in many other programming languages. But
what if we need to do `some_function(a, other_function(b))`? This doesn't do
that...

```python
# some_function(a, other_function, b) in many other languages
some_function a other_function b;
```

...because functions are first-class objects in Ö; that is, they can be
assigned to variables, passed as arguments to other function and so on, so the
above code calls `some_function` with three arguments and doesn't call
`other_function` at all. It should look like this:

```python
# some_function(a, other_function(b)) in many other languages
some_function a (other_function b);
```

So, let's go through that just to make sure you know it:

- If you don't care about the return value of a function, use a statement like
  `function a b c;`.
- If you want to do something with the return value, wrap the function call in
  parentheses, like `(function a b c)`.

It's really that simple: *always* use parentheses if you want to use the return
value for something, and *never* use them otherwise.

```python
var x = (some_function a b c);    # assigns the return value to x
```


## Infixes

The `equals` function takes two arguments, and it returns `true` if they are
equal and `false` otherwise. For example:

```python
if (equals 1 1) {
    print "this runs every time";
};
if (equals 1 2) {
    print "this never runs";
};
```

This code works, but `equals 1 2` is not very readable. You probably want to
read it as "1 equals 2", but it's forcing you to read it like "equals 1 2".
Infixes are the solution:

```python
if (1 `equals` 2) { ... };
```

Look carefully: `equals` is surrounded by *backticks* `` ` ``, not single
quotes `'`. So, ``a `f` b`` is just a handy shorthand, aka "syntactic sugar",
for `f a b`.


## Arrays

Ö arrays can be created like this:

```python
var thing = ["a" "b" "c"];
```

There are no fixed sizes; you can put as many elements as you want to an array.
If you want to display the content of the array, you need to use `debug`
instead of `print` as explained [above](#debugging).

```python
debug thing;     # prints ["a" "b" "c"]
print thing;     # throws an error!
```

You can check the length of an array like this:

```python
debug thing.length;     # prints 3 because there are 3 elements in the array
```

Here `thing.length` looks up an attribute, and the value of this attribute is
an [Integer](builtins.md#integer).

Most objects have many attributes whose values are functions; these attributes
are called methods. For example, arrays have a `push` method that adds an
element to the end of the array:

```python
debug thing.push;   # prints <Function "push method" at 0xblablabla>
thing.push "d";
debug thing;        # prints ["a" "b" "c" "d"]
```

[Click here](builtins.md#array) for a complete list of array attributes and
methods.


## Mappings

Unlike in many other languages, there's no nice syntax for creating mappings
yet. Mappings are created with the `new` function like this:

```python
var thingy = (new Mapping [["key 1" "value 1"] ["key 2" "value 2"]]);
```

Here `Mapping` is a class, and the `new` function creates a new instance of it.

The messy part is just an array of `[key value]` arrays. You can put anything
into an array, even other arrays.

Now we can access the values of our keys with the `get` method:

```python
print (thingy.get "key 2");   # prints "value 2"
```

[Click here](builtins.md#mapping) for a complete list of mapping methods and
more information about mappings.


## Scopes

If you create a variable in an `if`, you'll notice that it can't be accessed
after calling `if`:

```python
if true {
    var x = "lol";
    print x;
};
print x;    # error: no variable named 'x'
```

This is the behaviour you would expect to get with most programming languages,
but `Block` objects are really not limited in this way. In fact, you can run
*any* block object you want in *any* environment you want. In Ö, environments
of variables are called *scopes*, and they are represented with `Scope`
objects.

```python
var block = { print x; };
var x = "lol";
block.run block.definition_scope;     # prints "lol"
```

The `definition_scope` is the scope that Ö was running when `block` was
defined; that is, the scope that Ö is running our file in.

`Scope` objects have a `local_vars` attribute. It's a mapping with variable
name strings as keys.

```python
var this_scope = { }.definition_scope;
this_scope.local_vars.set "y" "lol wat";    # same as 'var y = "lol wat";'
print y;          # prints "lol wat"
```

Here `{ }` is a `Block` object, and its `.definition_scope` is a handy way to
access the scope that our code is running in.

Scopes also have a `parent_scope` attribute. When looking up a variable, like
the `y` in `print y;`, Ö first checks if the variable is in
`this_scope.local_vars`. If it's not, `this_scope.parent_scope.local_vars` is
checked, and if it's not there, `this_scope.parent_scope.parent_scope.local_vars`
is checked and so on. Eventually, Ö gets to the built-in scope; that is the
scope that built-in functions and other things like `true`, `null`, `new` and
`Mapping` are in. The `parent_scope` of the built-in scope is `null`.

It's also possible to create new Scopes; just create a `Scope` object with
`new`, and pass a parent scope as an argument:

```python
var subscope = (new Scope {}.definition_scope);
{ var z = "boom boom"; }.run subscope;
{ print z; }.run subscope;             # prints "boom boom"
print (subscope.local_vars.get "z");   # prints "boom boom"
print z;       # error because z is in the subscope
```

This is how `if` runs the block that is passed to it. I'll show you how to
write your own `if` in the next section.


## Defining Functions

The `func` function takes a string and a `Block` object (that is,
`{ something }`) as arguments. It works like this:

```python
func "print_twice string" {
    print string;
    print string;
};

print_twice "hello";
```

The string that `func` takes describes how the function must be called, and it
should contain variable names separated by spaces. All variable names except
the first are argument names of the function, and the resulting function ends
up in the block's `.definition_scope` as a variable named by the first variable
name in the string.

Function objects behave nicely with [the debug function](#debugging):

```python
debug print_twice;   # prints <Function "print_twice" at 0xblablabla>
debug print;         # prints <Function "print" at 0xblablabla>
debug func;          # prints <Function "func" at 0xblablabla>
```

You can pass any number of arguments you want to `func`.

```python
func "hello" {
    print "hello";
};
hello;

func "print3 a b c" {
    print a;
    print b;
    print c;
};
print3 "hello" "world" "test";
```

`func` runs the block passed to it like this:
1. A new subscope of the block's definition scope is created.
2. A local `return` variable of the subscope is set to `null`.
3. The block is ran in the subscope.
4. The current value of the `return` variable is returned.

To be honest, I don't like this `return` variable thing. I'll implement a
*real* return later. Meanwhile, if you want to return something from a
function, just set the `return` variable to something:

```python
func "asd" {
    return = "asd asd";
};

print (asd);    # prints "asd asd"
```

Note that returning doesn't end the function like it does in many other
programming languages, so this function...

```python
func "thingy" {
    return = 123;
    print "still alive";
};
```

...prints `still alive`.

It's time to write our own **pure-Ö implementation of if** without using the
built-in if at all! Let's do it.

```python
func "fake_if condition block" {
    # keys are conditions, values are blocks that should run in each case
    var mapping = (new Mapping [
        [true {
            # run the block in a new subscope of the scope it was defined in
            block.run (new Scope block.definition_scope);
        }]
        [false {
            # do nothing
        }]
    ]);

    (mapping.get condition).run {}.definition_scope;
};

fake_if (1 `equals` 1) {
    print "yay";
};
```

This is quite awesome IMO. Our `fake_if` is implemented purely in the language
itself, but it's still called exactly like the built-in `if`. Many "built-in"
functions are actually implemented in Ö; see
[stdlib/builtins.ö](../stdlib/builtins.ö) for their source code.


## Options

Functions can also take options. They are like arguments, but every option has
a name associated with it. Options are also *optional*, so you don't need to
specify them when calling the function.

Here's an example:

```python
func "thingy message twice:" {   # twice is an option
    if (twice `equals` null) {
        # the twice option wasn't given
        twice = false;
    };

    print message;
    if twice {
        print message;
    };
};

thingy "hello";              # prints hello
thingy "hello" twice:true;   # prints hello two times
```

The `twice:` in `"thingy message twice:"` looks a lot like the beginning of
`twice:true`, and that's why we add `:` at the end of an argument name to make
it an option.

Let's make a `better_fake_if` function that's like the `safe_if` we wrote
earlier, but with support for an `else:` option.

```python
# this is copied from the previous example
func "fake_if condition block" {
    var mapping = (new Mapping [
        [true { block.run (new Scope block.definition_scope); }]
        [false { }]
    ]);

    (mapping.get condition).run {}.definition_scope;
};

func "better_fake_if condition block else:" {
    fake_if condition {
        block.run (new Scope block.definition_scope);
    };
    fake_if (not condition) {
        fake_if (not (else `equals` null)) {
            else.run (new Scope else.definition_scope);
        };
    };
};

better_fake_if (1 `equals` 2) {
    print "yay";
} else: {
    print "nay";    # this runs because 1 is not equal to 2
};
```

The built-in `if` has a similar `else:` option.


## Defining Classes

Let's look at an example of a custom class in Ö.

```python
class "Foo" {
    method "setup" {
        print "creating a new Foo instance";
    };

    method "toot bar" {
        print ("tooting with ".concat bar);
    };
};


var f = (new Foo);      # prints "creating a new Foo instance"
f.toot "asd";           # prints "tooting with asd"
```

As you should have guessed by now, `class` is just a function...

```python
debug class;      # prints <Function "class" at 0xblablabla>
```

...and `Foo` is just a `Class` object, just like e.g. `Mapping`:

```python
debug Mapping;  # prints <Class "Mapping">
debug Foo;      # prints <Class "Foo">
```

`class "Foo" { ... };` creates a new subscope of `{ ... }.definition_scope` and
runs the block in it. The block may contain any code; it's just executed. So,
this...

```python
class "Bar" {
    print "hello";
};
```

...prints hello and gives you a class with no methods.

In the block, we used a function called `method`. It behaved a lot like `func`,
and we used it to add a `toot` method to our class. However, this doesn't
work...

```python
debug method;       # error: no variable named 'method'
```

...but this works:

```python
class "Baz" {
    debug method;   # prints <Function "debug" at 0xblablabla>
};
```

`method` is a function that is inserted to the subscope that the block is
running in.

In the `Foo` example, we defined the `setup` method like
`method "setup" { ... };`. If it takes arguments, those arguments must be
passed to `new`:

```python
class "Thing" {
    method "setup x y" {
        debug x;
        debug y;
    };
};

var thing = (new Thing "one" "two");   # prints "one" and "two"
```

You can define new attributes with `attrib`. It's only available inside the
block argument of `class`, just like `method`. It works like this:

```python
class "Tooter" {
    attrib "message";

    method "setup message" {
        this.message = message;
    };

    method "toot" {
        print ("Toot toot! ".concat this.message);
    };
};

var tooter = (new Tooter "hello there");
tooter.toot;     # prints "Toot toot! hello there"
```

The `this` variable is available in methods, and it's set to the instance that
the methods are called from. In this example, `this` was `tooter`.
