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
var thing = [1 2 3];
```

There are no fixed sizes; you can put as many elements as you want to an array.
Note that the `print` function wants a string, so convert arrays to strings
before printing them, e.g. `print (thing::to_string);`. Here `thing::to_string`
looks up a method; methods are just functions.

```python
print (thing::to_string);   # prints [1 2 3]
```

Arrays have more useful methods than `to_string`:

- `thing::push x;` adds `x` to the end of `thing`.
- `(thing::pop)` deletes and returns the last element of `thing`.
- `(thing::get_length)` returns the number of items in `thing`
- `(thing::get i)` returns the `i`'th element from `thing`. The indexes start
  at 0.
- `thing::set i x;` sets the `i`'th element of `thing` to `x`, ignoring its old
  value. `i` must be less than the length of `thing`.
- `(thing::slice i j)` returns a new array that contains the elements between
  `i` and `j`. Both `i` and `j` are interpreted so that `0` means the beginning
  of the array and `(thing::get_length)` means the end of the array. Too big or
  small indexes are ignored, but only with `slice`; not with other methods.
- `(thing::slice i)` returns `(thing::slice i (thing::get_length))`.

Here's an example:

```python
var stuff = ["a" "b" "c"];

stuff::push "d";
stuff::push "e";
print (stuff::to_string);   # ["a" "b" "c" "d" "e"]

print (stuff::get 3);       # d
stuff::set 3 "ddd";
print (stuff::to_string);   # ["a" "b" "c" "ddd", "e"]
print (stuff::pop);         # e
print (stuff::to_string);   # ["a" "b" "c" "ddd"]

print ((stuff::slice 1 3)::to_string);   #     ["b" "c"]
print ((stuff::slice 0 3)::to_string);   # ["a" "b" "c"]
print ((stuff::slice 1)::to_string);     #     ["b" "c", "ddd"]

# too big and small indexes are ignored:
print ((stuff::slice -10 1000)::to_string);     # ["a" "b" "c", "ddd"]
```


## Mappings

Unlike in many other languages, there's no nice syntax for creating mappings
yet. Mappings are created with the `new` function like this:

```python
var thingy = (new Mapping [["key 1" "value 1"] ["key 2" "value 2"]]);
```

The messy part is just a list of `[key value]` lists.

Now we can access the values of our keys with the `::get` method:

```python
print (thingy::get "key 2");   # prints "value 2"
```

The values of a mapping can be any objects, but `Mapping` is
[implemented](../src/objects/mapping.c) as [a hash
table](https://en.wikipedia.org/wiki/Hash_table), so the keys must be hashable.

Most objects are hashable in Ö, but arrays and mappings are not. If they were,
the hash value would need to change as the content of the array or mapping
changes, but objects that change their hash values on the fly would break most
things that use hash values, e.g. hash tables.

I'll probably create some kind of `ImmutableArray` some day. Immutable arrays
would be hashable.


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
block::run block.definition_scope;     # prints "lol"
```

Here `block.definition_scope` is an attribute of `block`; attributes are
accessed just like methods, but with `.` instead of `::`. The
`definition_scope` is the scope that Ö was running when `block` was defined;
that is, the scope that Ö is running our file in.

`Scope` objects have a `local_vars` attribute. It's a mapping with variable
name strings as keys.

```python
var this_scope = { }.definition_scope;
this_scope.local_vars::set "y" "lol wat";    # same as 'var y = "lol wat";'
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
{ var z = "boom boom"; }::run subscope;
{ print z; }::run subscope;             # prints "boom boom"
print (subscope.local_vars::get "z");   # prints "boom boom"
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

You can have any number of arguments you want:

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

Unfortunately there's no way to return anything from a function yet. :(

It's time to write our own **pure-Ö implementation of if** without using the
built-in if at all! Let's do it.

```python
func "fake_if condition block" {
    # keys are conditions, values are blocks that should run in each case
    var mapping = (new Mapping [
        [true {
            # run the block in a new subscope of the scope it was defined in
            block::run (new Scope block.definition_scope);
        }]
        [false {
            # do nothing
        }]
    ]);

    (mapping::get condition)::run {}.definition_scope;
};

fake_if (1 `equals` 1) {
    print "yay";
};
```

This is quite awesome IMO. Our `fake_if` implemented purely in the language
itself, but it's still called exactly like the built-in `if`. Many "built-in"
functions are actually implemented in Ö; see
[stdlib/builtins.ö](../stdlib/builtins.ö) for their source code.
