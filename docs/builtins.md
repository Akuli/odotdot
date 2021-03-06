# Built-in Functions and Classes

Ö's [built-in scope] contains things like basic data structures and commonly
needed functions. They are listed here.


## Functions

### debug

`debug x;` does the same thing as `print x.(to_debug_string);`. See
[Object](#object) for more info about the `to_debug_string` method and
[the debugging section of the tutorial].

### func

`func "name arg1 arg2 arg3" block;` defines a new function with arguments named
`arg1`, `arg2` and `arg3`. `func "name arg1 arg2 arg3" returning:true block;`
defines [a returning function](tutorial.md#different-kinds-of-functions).

The string argument of `func` is first split with the `split_by_whitespace`
string method; see [String's method documentation](#string). Everything except
the first part of the split result are argument names, and there may be 0 or
more of them. The resulting [Function object](#function) is then set to
`block`'s [definition scope] with the name from the string.

`func` itself takes a `returning` option that should be set to [true] or
[false], defaulting to [false]. If `returning` is `true`, the block will be ran
with return; see [Block](#block)'s `run_with_return` method.

When called, the function creates a new subscope of `block`'s
[definition scope], and inserts the values of the arguments there as local
variables. If the function is returning, a `return` function that behaves like
the `return` of [Block](#block)'s `run_with_return` is also added, so
`return foo;` inside the function makes the function return `foo` and exits it.
If a returning function doesn't call `return`, a [ValueError] is thrown when
the function is called.

The string argument to `func` may contain parts ending with `?`. They are names
of optional arguments, so `thing?` adds an option named `thing`. Calling the
function without specifying a `thing` runs the `block` so that `thing` is set
to `none`, and calling the function like `the_function thing:something;` runs
the block so that `thing` is set to `(new Option something)`. See also
[Option documentation](#option).

Optional arguments must be after non-optional arguments when defining the
function, but not when calling the function. For example,
`func "name arg1 opt? arg2" { ... };` throws [ValueError], but
`class "Someclass" inherits:AnotherClass { ... };` works.

**See Also:** There's more info about functions in the [Function](#function)
class documentation. The functions created by `func` are `Function` objects.

### lambda

`(lambda "arg1 arg2 arg3" block)` returns a new function.

`lambda` is like [func](#func), but it returns a function instead of setting it
to a variable in `block`'s definition scope automagically. This also means that
there's no function name in the string argument.

The `name` attribute (see the [Function](#function) class) is set to
`"a lambda function"` by default, but you can set that to any other string you
want.

### if

`if condition block;` runs the block if `condition` is [true].

If `condition` is [true], `block` will be ran in a new [subscope] of its
[definition scope]. [TypeError] is thrown if the `condition` is not [true] or
[false] or the block is not a [Block](#block) object.

`if` can also take another block with an optional `else` argument, and it's ran
in a new subscope of its definition scope if the condition turns out to be
false. For example:

```python
if (1 == 2) {
    print "something is very broken!";
} else: {
    print "seems to work";
};
```

### switch

This is ugly...

```python
var y = none;    # needed to bring y out of the if scopes
if (x == 1) {
    y = "a";
} else: {
    if (x == 2) {
        y = "b";
    } else: {
        if (x == 3) {
            y = "c";
        } else: {
            y = "d";
        };
    };
};
print y;
```

...and is best written with `switch` instead:

```python
var y = (switch x {
    case 1 { "a" };
    case 2 { "b" };
    case 3 { "c" };
    default { "d" };
});
print y;
```

Here the `{ "a" }` blocks use [implicit returns].

You can put any code you want to the `case` and `default` blocks:

```python
var y = (switch x {
    case 1 { print "a"; return "a"; };
    case 2 { print "b"; return "b"; };
    case 3 { print "c"; return "c"; };
    default { print "d"; return "d"; };
});
```

The `return`s are needed to prevent falling through; without them, both `a` and
`d` would be printed if `x` is `1`. [ValueError] is thrown if the switch
doesn't return anything.

If you want to run code without returning anything from the `switch`, use a
throw-away variable:

```python
var y = none;    # needed to bring y out of the switch scopes
var _ = (switch x {
    case 1 { y = "a"; return none; };
    case 2 { y = "b"; return none; };
    case 3 { y = "c"; return none; };
    default { y = "d"; return none; };
});
```

This is less idiomatic than `var y = (switch x { ... });`, but it can be useful
in some cases.

Here's a more detailed description of what `(switch x switchblock)` does:

1. A new subscope of `switchblock`'s definition scope is created. Let's call
   this scope the switch scope.
2. `case` and `default` functions are added to the switch scope. They work like
   this:
    - `case y block;` runs `block` in a new subscope of the switch scope if
      `(x == y)`.
    - `default block;` always runs `block` in a new subscope of the switch
      scope.
3. A `return` function that returns from the `switch` call is added to the
   switch scope.
4. `switchblock` is ran in the switch scope.

### not

`(not x)` returns [false] if `x` is [true], or [true] if `x` is [false].

If `x` is not [true] or [false], `(not x)` throws [TypeError].

### and

``(x `and` y)`` returns [true] if `x` is [true] and `y` is [true]. Otherwise it
returns [false].

`x` and `y` must both be [true] or [false]; otherwise a [TypeError] is thrown.

### or

``(x `or` y)`` returns [true] if `x` is [true], `y` is [true] or they are both
[true]. Otherwise it returns [false].

`x` and `y` must both be [true] or [false]; otherwise a [TypeError] is thrown.

### same_object

``(x `same_object` y)`` returns [true] if `x` and `y` point to the same object,
and [false] otherwise.

For example, this sets `x` and `y` to the same empty [array](#array)...

```python
var x = [];
var y = x;
```

...so you get behaviour like this:
- `x` and `y` are both empty, so `(x == y)` returns [true].
- ``(x `same_object` y)`` returns [true].
- If you do `x.push "hi";` and then `debug y;`, you'll get `["hi"]`. The `x`
  and `y` variables point to the same [array](#array), so doing something with
  `x` or `y` does that something to the array, regardless of the variable used.

But this creates two different objects...

```python
var x = [];
var y = [];
```

...and things behave a bit differently:
- `x` and `y` are both empty, so `(x == y)` still returns [true].
- This time, ``(x `same_object` y)`` returns [false].
- Doing something to `x` doesn't do the same thing to `y`.

### while

`while { condition } block;` runs `block` repeatedly until `condition`
evaluates to [false]. Here `{ condition }` is syntactic sugar for
`{ return condition; }`; see block expressions documented in
[the syntax spec](syntax-spec.md#parsing-to-ast).

`while` creates a new subscope of `block`'s [definition scope]. Then it runs
the first argument in it, and that should return [true] or [false]; if it
returns something else, [TypeError] is thrown. If [true] was returned, `block`
is ran in the same subscope. Then the first argument and the block are ran
again and the process is repeated until the first argument returns [false].

Unfortunately there's no `break` or `continue` yet.

Example:

```python
# print numbers from 0 to 9
var i = 0;
while { (i < 10) } {
    print i.(to_string);
    i = (i + 1);
};

# the i variable is not cleaned up
debug i;    # prints 10
```

### for

`for { init; } { condition } { increment; } { body };` is Ö's equivalent of the
traditional for loop; that is, *not* the foreach loop that some
languages call `for`. Here's an example:

```python
# print numbers 0 to 9
for { var i = 0; } { (i < 10) } { i = (i+1); } {
    print i.(to_string);
};

debug i;   # an error!
```

Everything is ran in a subscope, so unlike in the `while` example above, the
`i` isn't accessible anymore after the loop. Note that there's no `;` after
`(i < 10)`; see the while example above.

The `for` loop is more powerful than `while`. Everything that can be done with
`while` can also be done with `for`, but with `for` you can also have init and
increment blocks. In fact, `while` is implemented like this:

```python
func "while condition body" {
    for {} condition {} body;
};
```

Here's a more detailed description of how `for` works:
1. A new subscope of the body block's definition scope is created. All of the
   blocks given as arguments will always be ran in this scope.
2. The init block is ran.
3. The condition block is ran with return. See [Block](#block)'s
   `run_with_return` method.
4. If the condition block returned [false], `for` terminates without an error.
   Else, if it returned something else than [true], a [TypeError] is thrown
   (and `for` terminates).
5. The body block is ran.
6. The increment block is ran.
7. The condition is checked and body and increment are ran over and over again
   until `condition` returns [false].

As with `while`, there's no `break` or `continue` yet.

### catch and throw

See [the documentation about errors](errors.md).

### import

This function is documented [here](importing.md).

### assert

`assert x;` is equivalent to
`if (not x) { throw (new AssertError "assertion failed"); };`.

### new

`(new SomeClass arg1 arg2 arg3)` creates a new instance of a class.

The arguments are passed to the `setup` method of the class; that is, the
number and meanings of the arguments depends on which class was passed to
`new`.

### get_class

`(get_class x)` returns the [class](#classes) of `x`.

Don't use this function for checking types. For example,
``((get_class []) `same_object` Object)`` returns [false], even though
[all values are Objects](#classes) and therefore `[]` is also an Object. Use
[is_instance_of](#is_instance_of) instead.

### is_instance_of

``(x `is_instance_of` SomeClass)`` checks if `x` is an instance of the class.

For example, ``("hello" `is_instance_of` String)`` returns [true], and
``(123 `is_instance_of` Array)`` returns [false].

### class

This function creates a new class. See
[defining classes in the tutorial](tutorial.md#defining-classes).


## Classes

Every object has a class in Ö, and the class defines the attributes that the
object has. Methods are just attributes whose values are functions, so the same
thing applies to methods as well. For example, the class of `[1 2 3]` is
[Array](#array), and the Array class defines a method named `push` so you can
do `the_array.push 4;`.

In some languages, there are "primitives" that are not objects and cannot be
treated like objects. I think that's dumb, so everything you can e.g. assign to
a variable is an object in Ö, including everything from integers and strings to
functions and classes.

### Error and its subclasses

See [the documentation about errors](errors.md).

### Class

Classes are also objects; they are instances of a class named `Class`. The
`Class` class is not in the built-in namespace because most of the time you
don't need it, but if you do need it, you can do e.g.
`var Class = (get_class String);` because the `String` class is a `Class`
object.

Classes have these attributes:
- `some_class.name` is the name of the class as a [String](#string). This
  attribute is only for debugging, and it cannot be set after creating the
  class.
- `some_class.baseclass` is an [Option](#option) of the class that
  `some_class` inherits from (see the [*] note below). `Object.baseclass` is
  [none](#none).
- `some_class.getters` is a mapping with attribute name strings as keys and
  attribute getter functions (see below) as values.
- `some_class.setters` is a similar mapping as `getters` for setter functions.

When looking up any attribute, the interpreter looks up a getter function from
the instance's class, and calls it with the instance as the only argument. So,
`x.y` is equivalent[*] to `(((get_class x).getters.get "y") x)` (that is a
function call with `((get_class x).getters.get "y")` as the function and `x` as
the only argument).

Similarly, setting an attribute calls a setter function with two arguments, the
instance and the new value, so `x.y = z;` is equivalent[*] to
`((get_class x).setters.get "y") x z;`.

This getter and setter stuff has a few important consequences:
- All instances of a class have the same attributes.
- Read-only attributes can be implemented by adding a getter to `.getters` but
  no setter. For example, all methods and the `length` attribute of
  [Array](#array) objects work like this.
- It's possible to add more methods to built-in objects; the mappings that the
  attributes are stored in are not hidden, and there are no special
  restrictions for how those mappings can be used. However, as explained above,
  adding an attribute means adding it to **all** instances of a class, so
  modifying the `setters` or `getters` of built-in classes should be considered
  an anti-pattern.

[*] Not exactly. Here are the details:
- Inheritance makes this work a bit differently: the `setters` and `getters` of
  the base class are checked if the attribute is not found in `some_class`. If
  it's not in the base class either, the base class of the base class is
  checked and so on until the attribute is found or there are no more base
  classes.
- If the attribute is missing, an `AttribError` is raised with the `.` syntax,
  but a `KeyError` is raised when accessing `setters` or `getters` directly.

Classes also have one method:
- `some_class.(to_debug_string)` returns a string like `<Class "the name">`
  where `the name` is `some_class.name`. See also [Object](#object)'s
  `to_debug_string` documentation.

Usually new classes are created with [class](#class), but you can also create
them with `(new Class name baseclass)`, where `Class` is `(get_class String)`
(see above), `name` will become the `.name` attribute and `baseclass` is
another `Class` object to inherit from (usually [Object](#object)).

### Object

This is the baseclass of all other classes; that is, all other classes inherit
from this class and thus have the methods that this class has.

New objects can be created with [new](#new) like `(new Object)`, but that's not
very useful because `Object` is meant to be used mostly as a base class for
other things. However, ``(x `is_instance_of` Object)`` always returns [true]
regardless of the type of `x`, and that could be useful with e.g. something
that checks types with [is_instance_of](#is_instance_of).

Methods:
- `object.(to_debug_string)` should return a programmer-friendly string that
  describes the object. [The debug function](#debug) uses this. `Object`'s
  `to_debug_string` returns a string like `<ClassName at 0xblablabla>` where
  `ClassName` is the name of the object's class and the `0xblablabla` part is
  different for each object.

Many objects also have a `to_string` method that should return a human-readable
string, but not all objects have a human-readable string representations, even
though all objects *must* have a programmer-readable string representation.

### ArbitraryAttribs

Attributes of this class behave differently than attributes of other classes.

[comment]: # (TODO: make "Class" a link)

If `aa` is an `ArbitraryAttribs` object, then `aa.x = some_value;` first tries
to look up a setter for an `x` attribute as described in `Class` documentation,
but if that fails, no `AttribError` is thrown; instead, the value goes to a
special [mapping](#mapping) associated with the instance. Similarly, looking up
`aa.x` checks that mapping if no getter is found, and throws `AttribError` only
if the special mapping doesn't contain the key. Here's an example:

```python
var aa = (new ArbitraryAttribs);
aa.lol = "wat";   # works only with ArbitraryAttribs instances
print aa.lol;     # prints "wat"
```

Subclasses of `ArbitraryAttribs` also behave this way.

### Option

**See also:** There's [a more detailed introduction to
options](tutorial.md#option-objects).

Option objects should be used every time something may be `none`. This way you
won't forget to check the `none` case because accessing the value requires an
explicit `thingy.(get_value)`. `none` is a special `Option` object that represents
the no value case.

New options can be created with `(new Option the_value)`. New options with no
value cannot be created; just use `none`.

Options can be compared with `==`. It works like this:
- If both options are `none`, `true` is returned.
- If only one of the options is `none`, `false` is returned.
- If neither of the options are `none`, the values are compared with `==`.

Note that `(some_option == none)` doesn't do anything with the value of the
option. This means that you can use `==` and `!=` to check if an option is
`none`, no matter what the value of the option is when it's not `none`.

Methods:
- `option.(get_value)` returns the value of the option, or throws [ValueError]
  if `option` is `none`.
- `option.(get_with_fallback default)` returns `default` if `option` is `none`,
  and the value of the option otherwise.
- `option.(to_debug_string)` returns a string like `"<Option: valuestring>"`
  where `valuestring` is `option.(get_value).(to_debug_string)`.
  `none.(to_debug_string)` returns `"none"`. See [Object](#object)'s
  `to_debug_string`.

### Bool

There are two `Bool` objects, `true` and `false`. Creating more Bools wouldn't
make sense, so `(new Bool)` always throws an error. This means that
``(x `is_instance_of` Bool)`` means
``((x `same_object` true) `or` (x `same_object` false))``

Methods:
- `true.(to_debug_string)` returns `"true"` and `false.(to_debug_string)`
  returns `"false"`. This overrides [Object](#object)'s `to_debug_string`.

**See also:** [and](#and), [or](#or), [not](#not), [if](#if)

### String

New strings cannot be created with `(new String)`; use *string literals* like
`"hello"` instead. There are more details about string literals
[here][string literals].

Strings are immutable, so they cannot be changed after creating the string;
there is no [array](#array)-like `set` method even though there's a `get`
method. Create a new string instead of modifying existing strings.

Strings can be compared with `==` as you would expect; `("asd" == "asd")`
returns `true`, and `("asd" == "ass")` returns `false`. Don't use
[same_object](#same_object) for comparing strings; it *may* work in some
special cases, but ``(string1 `same_object` string2)`` is not *guaranteed* to
have any meaningful value. tl;dr: don't do it.

Strings behave a lot like [arrays](#array) of strings of length 1. For example,
`(["h" "e" "l" "l" "o"].get 3)` and `("hello".get 3)` both return `"l"`. To be
more precise, `String` is a subclass of [FrozenArrayLike].

Strings can be concatenated with the `+` operator: `("hello" + "world")`
returns `"helloworld"`.

Methods:
- `string.get` is like the array `.get` method.
- `string.slice` is like the array `.slice` method, but it returns a string
  instead of an array of characters.
- `string.length` is like the array `.length` attribute; it is the number of
  characters in the string as an [Integer](#integer).
- `string.(split_by_whitespace)` splits the string into an array of substrings
  separated by one or more Unicode whitespace characters (e.g. spaces, tabs or
  newlines). For example, `" hello world test ".(split_by_whitespace)` returns
  `["hello" "world" "test"]`.
- `string.(split separator)` returns an [Array](#array) of parts of
  `string` that have `separator` between them. For example,
  `"asda".(split "sd")` returns `["a" "a"]`, and `"asda".(split "kk")`
  returns `["asda"]`. [ValueError] is thrown if the `separator` is `""`.
- `string.(replace old new)` returns a new String with all occurences of an
  `old` string replaced by a `new` string. `ValueError` is thrown if `old` is
  `""`. The parts of the string that have already been replaced by `new` are
  never replaced again, e.g. `"xxxy".(replace "xy" "yy")` returns `"xxyy"` even
  though it contains the `"xy"` string. Unfortunately there's no way to replace
  multiple things at once yet :(
- `string.(to_string)` returns the `string` unchanged. This is for consistency
  with the `to_string` methods of other classes; `to_string` is supposed to
  return a human-readable string representing the object, if any, and the
  human-readable string representing a string is the string itself.
- `string.(to_debug_string)` returns the string with quotes around it. This
  overrides [Object](#object)'s `to_debug_string`.
- `string.(to_byte_array encoding_name)` [encodes] the string. The
  `encoding_name` is interpreted as if passed to [encodings.get].

Missing features:
- There are very little methods; there's no way to e.g. make the string
  uppercase or join by a separator efficiently.
- There's no string formatting, so you need to do
  `((((a + ", ") + b) + " and ") + c)`. I know, it's hard to get right and ugly
  and unmaintainable and bad in every possible way.

### ByteArray

A byte is an [Integer](#integer) between 0 and 255, and a `ByteArray` object
represents a sequence of bytes. Many things, like files and network I/O, store
everything as sequences of bytes, and these bytes are represented with
`ByteArray` objects in Ö.

`ByteArray` objects take up much less memory than [Array](#array)s of
[Integer](#integer)s, and that's why there is a separate class for sequences of
bytes in the first place. For example, if you have 1GB of data and 2GB of RAM,
the 1GB should fit just fine in a `ByteArray`, but you'll run out of RAM if you
try to convert the `ByteArray` to an [Array](#array) of [Integer](#integer)s.

However, even though `ByteArray`s aren't [Array]s, you can pass `ByteArray`s to
most functions that work with [Array]s because `ByteArray` inherits from
[FrozenArrayLike].

It's possible to represent string as bytes, and that's how text can be saved to
files. See [String](#string)'s `to_byte_array` method and `ByteArray`'s
`to_string` method.

`ByteArray`s can be concatenated with `+`. However, note that it takes up a lot
of memory if the `ByteArray`s are big; adding together a 1GB `ByteArray` with a
2GB `ByteArray` does **not** consume 3GB of RAM, it consumes 6GB temporarily;
first the `ByteArray` is created so that we have that and the old `ByteArray`s
(using 1GB + 2GB + 3GB of RAM), and if the old `ByteArray`s aren't used
anywhere else, they are then destroyed, bringing the memory usage back to 3GB.

`(new ByteArray integer_array)` creates a new `ByteArray` object from an
[Array](#array) from [Integer](#integer) values of bytes, throwing `ValueError`
if any of the [Integer](#integer)s are smaller than 0 or greater than 255.
`ByteArray` objects behave a lot like the integer arrays that they can be
created from, but they don't have methods like `push` or `set`; you cannot
change the content of a `ByteArray` after creating a `ByteArray`.

`ByteArray` objects can be compared with `==`, but it can be very slow if *all*
of the following are true:
1. the `ByteArray`s are different objects (i.e. [same_object](#same_object)
   returns `false`, so if both arrays are 1GB in size, together they consume 2GB
   of RAM) **AND**
2. the `ByteArray`s are of the same lengths **AND**
3. the `ByteArray`s have identical content or the content differs only near the
   end.

In this corner case with **all** these conditions, the arrays need to be
compared *byte by byte*. A 1GB `ByteArray` contains 1 billion bytes. Comparing
them with another 1GB `ByteArray`  will take a long time, but on the other
hand, these corner cases don't occur very often.

Methods:
- `bytearray.get` is like the array `.get` method.
- `bytearray.slice` is like the array `.slice` method, but it returns another
  `ByteArray` instead of an [Array](#array). The bytes are copied to a news
  `ByteArray`, so if you have a 2GB `ByteArray` and you take a 1GB slice of it,
  you use 3GB of RAM. If this is a problem for you,
  [let me know](https://github.com/Akuli/odotdot/issues/new) and I'll fix this.
- `bytearray.(split separator)` is like [String](#string)'s `split`, but
  it works with `ByteArray` objects instead of strings.
- `bytearray.(to_string encoding_name)` [decodes] the `ByteArray`. The
  `encoding_name` is interpreted as if passed to [encodings.get].

Attributes:
- `bytearray.length` is the number of bytes in the ByteArray.

Missing features:
- There's no nice `to_debug_string`, so if you `debug` a `ByteArray`, the
  output doesn't actually show the bytes in it.

### Integer

New integers can be created with integer literals like `123`, or with a
`(new Integer)` call like `(new Integer "123")`. The `new` call allows leading
zeros so that `(new Integer "-00000000012")` returns `-12`, but leading zeros
are not allowed in integer literals; see [this thing][integer literals] for
more details about integer literals.

Integers are immutable; see
[String](#string) for more details about what this means practically.

The smallest allowed value of an integer is
-(2<sup>63</sup> - 1) = -9223372036854775807, and the biggest value is
2<sup>63</sup> - 1 = 9223372036854775807. I'm planning on implementing
`Integer` differently so that it won't have these restrictions.

Methods (`x` and `y` are Integers):
- `x.(to_string)` converts the integer to a string without leading zeros.
- `x.(to_debug_string)` returns `x.(to_string)`. This overrides
  [Object](#object)'s `to_debug_string`.

The following operators work with integers like you would expect them to work,
returning Integer or [Bool](#bool) objects.

| Ö Code        | Mathematical Notation     |
| ------------- | ------------------------- |
| `(x + y)`     | x + y                     |
| `(x - y)`     | x - y                     |
| `(x * y)`     | xy                        |
| `(x == y)`    | x = y                     |
| `(x < y)`     | x < y                     |
| `(x > y)`     | x > y                     |
| `(x <= y)`    | x ≤ y                     |
| `(x >= y)`    | x ≥ y                     |

Missing features:
- There's no `/` because it would require floats, `Fraction` objects or integer
  division. Fraction objects and integer division would be doable, but I think
  floats are usually what people want. I'll probably add floats to Ö
  eventually.
- There's no support for other bases than 10. I think there's no need to add
  special syntax (`0xff` is useful in low-level languages like C, but confuses
  people in high-level languages), but syntax like `(new Integer "ff" 16)`
  would be useful for e.g. dealing with hexadecimal colors. Similarly,
  `255.(to_string 16)` could return `"ff"`.

### Array

Arrays represent ordered lists of elements that can be changed; it is possible
to add or remove items without creating a new array.

New arrays cannot be created with `(new Array)`. Use `[` and `]` instead:

```python
var array = [1 2 3];
```

`[]` creates an empty array.

There are no size limits; you can add as many elements to an array as you want
(as long as the array fits in the computer's memory).

Arrays can be compared with `==`, and two arrays compare equal if and only if
they are of the same length, and they have equal elements in the same order.

If you want to create your own object that behaves just like an array, create a
class that inherits from [ArrayLike]. `Array` inherits it too.

Attributes:
- `array.length` is the number of elements in the array as an
  [Integer](#integer).

Methods:

- `array.foreach varname { some code; };` creates a [subscope] of the
  block's [definition scope] and runs the block once for each element of
  the array, setting a variable named `varname` to the element. I'm
  sorry, there's no `break` or `continue` yet :(
- `array.push item;` adds `item` to the end of the array.
- `array.(pop)` deletes and returns the last item from the end of the array.
- `array.(get i)` returns the `i`'th element from the array. `array.(get 0)`
  is the first element, `array.(get 1)` is the second and so on. An error is
  thrown if `i` is negative or not less than `array.length`.
- `array.set i value;` sets the `i`'th item of the array to `value`. See `get`
  for more information about the `i` index.
- `array.(slice i j)` returns a new array that contains the elements between
  `i` and `j`. Both `i` and `j` are interpreted so that `0` means the beginning
  of the array and `length` means the end of the array. Too big or small
  indexes are "rounded" to the closest possible values.
- `array.(slice i)` returns `array.(slice i array.length)`.
- `array.(to_debug_string)` calls `to_debug_string` methods of the array
  elements and returns the debug strings in square brackets, joined with
  spaces. This overrides [Object](#object)'s `to_debug_string`.

Annoyances:
- There's no `array.map` method or a built-in `map` function, but you can
  easily define a `map` function like this:

    ```python
    func "map function array" {
        var mapped = [];
        array.foreach "item" {
            mapped.push (function item);
        };
        return mapped;
    };
    ```

**See also:** [same_object](#same_object)

### Mapping

Mapping objects *map* keys to values; that is, looking up the value
corresponding to a key is easy and fast.

New Mappings can be created in a few different ways:
1. `(new Mapping)` creates an empty mapping.
2. `(new Mapping pair_array)` takes an array of `[key value]` pair arrays and
   adds the keys and values to the mapping.
3. [Optional arguments][optional arguments] can be used together with 1. or 2. to add values with
   string keys to the mapping.

For example, the following two lines are equivalent:

```python
var ordering_words = (new Mapping [["first" 1] ["second" 2] ["third" 3]]);   # uses 2.
var ordering_words = (new Mapping first:1 second:2 third:3);       # uses 1. and 3.
```

The values in the mapping can be any objects, but the keys can't. `Mapping` is
implemented as a [hash table], so the keys need to be hashable.

Most objects are hashable in Ö, but [Arrays](#array) and Mappings are not. If
they were, the hash value would need to change as the content of the array or
mapping changes, but objects that change their hash values after creating the
object would break most things that use hash values, e.g. hash tables.

I'll probably create some kind of `ImmutableArray` some day. Immutable arrays
would be hashable.

Attributes:
- `mapping.length` is the number of key-value pairs in the mapping as an
  [Integer](#integer).

Methods:
- `mapping.set key value;` adds a new key-value pair to the mapping or changes
  the value of a key that is already in the mapping.
- `mapping.(get key)` returns the value of a key or throws an error if the key
  is not found.
- `mapping.delete key;` deletes a key-value pair from the mapping. An error is
  thrown if the key is not found.
- `mapping.(get_and_delete key)` is like `.delete`, but it returns the value
  of the deleted key.

Missing features:
- There's no way to loop over all the items in the mapping.

See [MappingLike] and [FrozenMappingLike] if you want to implement your
own object that behaves like `Mapping`. The `Mapping` class inherits
from [MappingLike].

### Block

**See Also:** [The Ö tutorial](tutorial.md) contains lots of examples of how
Blocks are used in Ö.

New Blocks can be created with `{ ... }` syntax or with
`(new Block definition_scope ast_statements)`. Here `definition_scope` should
be a [Scope object][scope], and `ast_statements` should be an [array](#array)
of [AstNode] objects. I'm not sure when the `new` call would be actually
useful, but it might be fun for something crazy like creating new
`AstNode` objects (not possible yet, see [this thing][AstNode]) and running
them.

Attributes:
- `block.ast_statements` is a list of
  [AstNode statement objects](syntax-spec.md#parsing-to-ast).
- `block.definition_scope` is
  [the scope that the block was defined in][definition scope] if the block was
  created with the `{ ... }` syntax, and the scope passed to `new Block`
  otherwise. For example, `{}.definition_scope` is a handy way to access the
  scope that the code is running in.

Methods:
- `block.run scope;` runs the AST statements in the scope passed to this
  function. This method does nothing with `definition_scope`.
- `block.(run_with_return scope)` inserts a `return` function to the scope's
  local variables and runs the scope as with `block.run scope;`. The `return`
  function takes 1 arguments and throws a [MarkerError] to stop the running.
  `run_with_return` then catches that `MarkerError` (but ignores all other
  `MarkerError`s, only the [same](#same_object) `MarkerError` object is
  caught) and returns the value that was passed to the `return` function. If
  `return` was not called, a [ValueError] is thrown.

### Scopes

**See Also:** The Ö tutorial has [a section just about
scopes](tutorial.md#scopes), and it explains `parent_scope` and the built-in
scope in more detail.

New scopes can be created with `(new Scope parent_scope)`. The `local_vars`
mapping will be initialized to an empty mapping. Use `{}.definition_scope` to
access the scope that your code is currently running in (see [Block](#block)
above).

Attributes:
- `scope.local_vars` is a [Mapping](#mapping) of local variables in the scope
  with variable name strings as keys. `{ var x = 123; }.run scope;` is
  equivalent to `scope.local_vars.set "x" 123;`.
- `scope.parent_scope` is an [Option](#option) of the scope that non-local
  variables are looked up from (see below), or [none](#none) if `scope` is the
  built-in scope.

For example, you can access the built-in scope like this:

```python
var builtin_scope = {}.definition_scope;
while { (builtin_scope.parent_scope != none) } {
    builtin_scope = builtin_scope.parent_scope.(get_value);
};
# now builtin_scope.local_vars.(get "while") works
```

Methods:
- `scope.set_var varname value;` sets the value of an existing variable in the
  scope, or if it's not found, in the parent scope or its parent scope and so
  on. `{ x = 123; }.run scope;` without `var` in front of `x` is equivalent to
  `scope.set_var "x" 123;`.
- `scope.(get_var varname)` looks up a variable similarly to `set_var`, but
  returns it instead of changing its value. `{ print x; }.run scope;` is
  equivalent to `print scope.(get_var "x");`.

### Function

The `Function` class is not in the built-in namespace, but you can access it
like `var Function = (get_class print)` if you need to. New functions cannot be
created with `(new Function)`; use [func](#func) instead.

If a function is called with the wrong number of arguments or with unknown
options, [ArgError] is thrown.

Attributes:
- `function.name` is a human-readable string for debugging. It can be e.g.
  `"print"` or `"to_string method"`. You can set the `.name` of any function to
  any string; this is not considered a problem because `.name` is just for
  debugging anyway.
- `function.returning` is [true] or [false]. The tutorial
  [explains this nicely](tutorial.md#different-kinds-of-functions).

Methods:
- `function.(partial arg1 arg2 arg3)` returns a new function that calls the
  original `function` with `arg1 arg2 arg3` and any arguments passed to the new
  function as arguments. So, `var g = (f.partial a b); g x y;` runs
  `f a b x y;`.
- `function.(to_debug_string)` returns a string like
  `<Function "the name of the function is here">`. See also the `.name`
  attribute above. This overrides [Object](#object)'s `to_debug_string`.


## Other objects in the built-in namespace

These objects are not functions or classes.

### none

This is a special `Option` object. See [the Option documentation](#option).


[true]: #bool
[false]: #bool
[scope]: tutorial.md#scopes
[subscope]: tutorial.md#scopes
[built-in scope]: tutorial.md#scopes
[definition scope]: tutorial.md#scopes
[AstNode]: syntax-spec.md#parsing-to-ast
[string literals]: syntax-spec.md#tokenizing
[integer literals]: syntax-spec.md#tokenizing
[hash table]: https://en.wikipedia.org/wiki/Hash_table
[optional arguments]: tutorial.md#optional-arguments
[the debugging section of the tutorial]: tutorial.md#debugging
[implicit returns]: tutorial.md#returning
[encodes]: std/encodings.md
[decodes]: std/encodings.md
[encodings.get]: std/encodings.md#get

[ArgError]: errors.md
[TypeError]: errors.md
[ValueError]: errors.md
[MarkerError]: errors.md

[FrozenArrayLike]: std/collections.md#frozenarraylike
[ArrayLike]: std/collections.md#arraylike
[FrozenMappingLike]: std/collections.md#frozenmappinglike
[MappingLike]: std/collections.md#mappinglike
