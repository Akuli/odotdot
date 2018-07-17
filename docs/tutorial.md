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

So far we have called functions like this:

```python
some_function a b;
```

This is like `some_function(a, b);` in many other programming languages. But
what if we need to use the return value for something? In many other
programming languages you can do things like
`some_function(a, other_function(b))`. This doesn't do that...

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

- If the function has no return value (see below), use a statement like
  `function a b c;`.
- If the function returns something, wrap the function call in parentheses,
  like `(function a b c)`.

It's really that simple: *always* use parentheses if you want to use the return
value for something, and *never* use them otherwise.

```python
var x = (some_function a b c);    # assigns the return value to x
```


## Different Kinds of Functions

Ö has two kinds of functions that behave differently:
- *Returning functions* return a value, and they must be called like
  `(function a b c)`.
- *Non-returning functions* must be called like `function a b c;`.

If you call a returning-function like `function a b c;` or a non-returning
function like `(function a b c)`, you'll get an error message.

This helps to avoid bugs. Here's a typical Python mistake:

```python3
message = "hello world"
message.split()
print(message)     # prints "hello world", not ["hello", "world"]
```

The message didn't get split because `message.split()` returns its result as a
new list, leaving `message` as it is. However, if you try to do the same thing
in Ö, like this...

```python
var message = "hello world";
message.split_by_whitespace;
debug message;
```

...it'll fail, which is *much* better than silently doing the wrong thing:

```
TypeError: expected a function that returns nothing, got <Function "split_by_whitespace method">
  in file /some/path/to/the/file.ö, line 2
```

Here is the correct code.

```python3
var message = "hello world";
var splitted = (message.split_by_whitespace);
debug splitted;    # prints ["hello" "world"]
```

Or without a temporary `splitted` variable:

```python3
var message = "hello world";
debug (message.split_by_whitespace);
```

Here `message.split_by_whitespace` is a method; we'll talk more about methods
[later in this tutorial](#arrays).

In the buggy Python code, the return value of `message.split()` was implicitly
ignored. If you want to ignore a return value in Ö, you need to do it
explicitly by creating a dummy variable:

```python3
var _ = (message.split_by_whitespace);
```

Here `_` is just an idiomatic variable name for throw-away variables.

By convention, returning functions are documented like
"`(the_function argument1 argument2)` does blah blah" and non-returning
functions are documented like "`the_function argument1 argument2;` does blah
blah".


## Infixes

The `is_instance_of` function can be used for checking types of objects like
this:

```python
if (is_instance_of thingy String) {
    print "it's a string";
};
```

This code works, but `is_instance_of thingy String` is not very readable. You
probably want to read it as "`thingy` is an instance of `String`", but it's
forcing you to read it like "is instance of `thingy` `String`". Haskell-style
infixes are the solution:

```python
if (thingy `is_instance_of` String) {
    print "it's a string";
};
```

Look carefully: `is_instance_of` is surrounded by *backticks* `` ` ``, not
single quotes `'`. So, ``(a `f` b)`` is just a handy shorthand, aka "syntactic
sugar", for `(f a b)`.


## Operators

Operators follow similar rules as infixed function calls (see above). You need
to parenthesize a function call every time you want to use its return value for
something, and the same rule applies to operators calls: you need to
parenthesize them.

```python
debug ("hello" + "world");  # prints "helloworld"
debug (1 + 2);              # prints 3
debug ((1 + 2) == 4);       # prints false
```

Note that `(1 + 2 == 4)` is invalid syntax, and we need `((1 + 2) == 4)`
instead. Ö doesn't know anything about operator precedence; sometimes that
makes operators a little bit more painful to use, but this way it's much easier
to create an Ö interpreter.

Ö supports these operators with similar meanings as in many other programming
languages: `==`, `!=`, `>`, `<`, `>=`, `<=`, `+`, `-`, `*`, `/`.


## Arrays

Ö arrays can be created like this:

```python
var thing = ["a" "b" "c"];
```

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

Most objects have many attributes whose values are functions. These attributes
are called *methods*. For example, arrays have a `push` method that adds an
element to the end of the array:

```python
debug thing.push;   # prints <Function "push method">
thing.push "d";
debug thing;        # prints ["a" "b" "c" "d"]
```

There are no fixed sizes; you can put as many elements as you want to an array.

Arrays can be compared with `==` and `!=`:

```python
debug (thing == ["a" "b" "c" "d"]);           # prints true
debug (thing == ["a" "b" "c" "dd lol wat"]);  # prints false
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


## Option Objects

In many other languages, an object often named `null` or `None` represents a
"nothing" value, so you can write code like this Python code:

```python3
try:
    website = get_website()
except WebsiteError:
    website = None

if website is None:
    print("No website was specified! Cannot print the URL.")
else:
    print(website.url)
```

It's also easy to forget the `None` check:

```python3
print(website.url)
```

If `website` happens to be `None`, you'll get an error for accessing
`None.url`. The problem with this is that you don't know whether `website` can
be `None` by looking at just the `print(website.url)` line. Your program might
work for a long time and *then* break unexpectedly.

Ö has a `none` object as well, but **don't use it** like Python's `None`. The
equivalent Ö code should look like this:

```python3
var website = none;
catch {
    website = (new Option (get_website));
} WebsiteError { };   # do nothing on WebsiteError because website is already set to none

if (website == none) {
    print "No website was specified! Cannot print the URL.";
} else: {
    print (website.get_value).url;
};
```

**If some value may be `none`, wrap that value in an `Option` object whenever
it's not `none`.** Option objects have a `get_value` method that returns the
value passed to `new Option`, and they can be conveniently compared with `==`.

Here is the broken code:

```python3
print (website.get_value).url;
```

It's *very* easy to see that something's wrong here because the `.get_value`
method is called without a `none` check.

Option objects have more convenient attributes and methods. See [the Option
documentation](builtins.md#option) for more details.


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

If you want to use the `x` outside the `if`, do this:

```python
var x = "default value";
if true {
    x = "lol";
};
print x;   # prints lol
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

Scopes also have a `parent_scope` attribute, which is an
[Option](#option-objects) of a `Scope`. When looking up a variable, like the `y`
in `print y;`, Ö first checks if the variable is in `this_scope.local_vars`. If
it's not, `(this_scope.parent_scope.get_value).local_vars` is checked, and if
it's not there,
`((this_scope.parent_scope.get_value).parent_scope.get_value).local_vars` is
checked and so on. Eventually, Ö gets to the built-in scope; that is the scope
that built-in functions and other things like `true`, `none`, `new` and
`Option` are in. The `parent_scope` of the built-in scope is `none`, and that's
why `parent_scope` is an [Option](#option-objects).

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

Functions defined like `func "name args" { ... };`run in a new
[subscope](#scopes) of `{ ... }.definition_scope`.

```python
func "thingy" {
    var x = "hi";
};

thingy;
print x;      # fails!
```

The string that `func` takes describes how the function must be called, and it
should contain variable names separated by spaces. All variable names except
the first are argument names of the function, and the resulting function ends
up in the block's `.definition_scope` as a variable named by the first variable
name in the string.

Function objects behave nicely with [the debug function](#debugging):

```python
debug print_twice;   # prints <Function "print_twice">
debug print;         # prints <Function "print">
debug func;          # prints <Function "func">
```

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

fake_if (1 == 1) {
    print "yay";
};
```

This is quite awesome IMO. Our `fake_if` is implemented purely in the language
itself, but it's still called exactly like the built-in `if`. Many "built-in"
functions are actually implemented in Ö; see
[std/builtins.ö](../std/builtins.ö) for their source code.


## Returning

You can return values a lot like in many other programming languages, but you
must say explicitly that you want to define a
[returning function](#different-kinds-of-functions):

```python
func "asd" returning:true {
    return "asd asd";
    print "this never gets printed, returning ends the function";
};

print (asd);    # prints "asd asd"
```

Here `returning:true` is an *optional argument*; we'll talk more about these
[soon](#optional-arguments).

The `return` function is just a local variable added to the scope that the code
runs in. An error is thrown if `return` is never called.

There's also some syntactic sugar: `{ value }` without a `;` is equivalent to
`{ return value; }`.

```python
func "lol" returning:true { "asda" };
print (lol);    # prints "asda"
```

This is useful with functions like [while](builtins.md#while):

```python
# print numbers 0 to 9
var i = 0;
while { (i < 10) } {
    print (i.to_string);
    i = (i + 1);
};
```

Here `{ (i < 10) }` returns the value of `(i < 10)` to the `while` function.
The parentheses are needed as explained [above](#operators).

Rust-style `{ something; the_return_value }` syntax is not supported because
`{ value }` is meant to be used when writing `return` everywhere would be
annoying, not as a replacement to `{ return value; }`. In other words,
`{ return value; }` is not bad style in Ö like it is in rust.


## Optional Arguments

Here's some code that you should be able to understand with what we have
learned so far.

```python
func "repeat string how_many_times" returning:true {
    var result = "";
    for { var i=0; } { (i < how_many_times) } { i = (i+1); } {
        result = (result + string);
    };
    return result;
};

func "print_box string" {
    print (repeat "*" (string.length + 4));
    print (("* " + string) + " *");
    print (repeat "*" (string.length + 4));
};

print_box "hello";
```

Here's the output:

```
*********
* hello *
*********
```

What if you want to print a box with some other character instead of `*`? You
could define the function like this...

```python
func "print_box string border" {
    var prefix = (border + " ");
    var suffix = (" " + border);

    print (repeat border (string.length + 4));
    print ((prefix + string) + suffix);
    print (repeat border (string.length + 4));
};
```

...and call it with different borders:

```python
print_box "hello" "*";
print_box "money" "$";
```

But now we can't do `print_box "hello";` anymore because we'll get an error for
not giving the border argument. We can still do that if we make the border
*optional* by putting a `?` after it when defining the function:

```python
func "print_box string border?" {
    var border_value = (border.get_with_fallback "*");
    var prefix = (border_value + " ");
    var suffix = (" " + border_value);

    print (repeat border_value (string.length + 4));
    print ((prefix + string) + suffix);
    print (repeat border_value (string.length + 4));
};
```

The `border` will be `none` if the function is called like
`print_box "hello";`, and an [Option](#option-objects) of the border if it is
given. `Option` objects have a `get_with_fallback` method that takes 1 argument
and returns the value of the option, or the argument if the option is `none`.
Now we can call the function like this:

```python
print_box "hello";              # uses "*" as the border
print_box "hello" border:"*";   # uses "*" as the border
print_box "money" border:"$";   # uses "$" as the border
```

Let's make a `better_fake_if` function that's like the `fake_if` we wrote
earlier, but with support for an optional `else:` argument.

```python
# this is copied from the fake_if example
func "fake_if condition block" {
    var mapping = (new Mapping [
        [true { block.run (new Scope block.definition_scope); }]
        [false { }]
    ]);

    (mapping.get condition).run {}.definition_scope;
};

func "better_fake_if condition block else?" {
    fake_if condition {
        block.run (new Scope block.definition_scope);
    };
    fake_if ((not condition) `and` (else != none)) {
        (else.get_value).run (new Scope (else.get_value).definition_scope);
    };
};

better_fake_if (1 == 2) {
    print "IT BROKE  :(";
} else: {
    print "wörks";
};
```

The built-in `if` can take an `else:` argument that works similarly.


## Defining Classes

Let's look at an example of a custom class in Ö.

```python
class "Foo" {
    method "setup" {
        print "creating a new Foo instance";
    };

    method "toot bar" {
        print ("tooting with " + bar);
    };
};


var f = (new Foo);      # prints "creating a new Foo instance"
f.toot "asd";           # prints "tooting with asd"
```

As you should have guessed by now, `class` is just a function...

```python
debug class;      # prints <Function "class">
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
    debug method;   # prints <Function "method">
};

# now Baz is a class with no methods
```

`method` is a function that is inserted to the scope that the block is running
in.

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
        print ("Toot toot! " + this.message);
    };
};

var tooter = (new Tooter "hello there");
tooter.toot;     # prints "Toot toot! hello there"
```

The `this` variable is available in methods, and it's set to the instance that
the methods are called from. In this example, `this` was `tooter`.

You can also do inheritance a lot like in other languages:

```python
class "FancyTooter" inherits:Tooter {
    method "fancy_toot" {
        print "*** Fancy Fanciness ***";
        this.toot;
    };
};

var tooter2 = (new FancyTooter "hello there");
tooter2.fancy_toot;
```

Multiple inheritance is not supported, only one class can be inherited. There's
also no way to call a superclass method yet :(

Adding `abstract "thingy";` to the class body defines a `"thingy"` attribute
that throws `AttribError` when it's looked up.

```python
class "AbstractTooter" {
    abstract "toot";    # meant to be overridden in a subclass
};

(new AbstractTooter).toot;   # AttribError
```

Use `abstract "setup";` if you want to prevent creating instances of the class
without subclassing.
