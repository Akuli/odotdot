# Built-in Functions and Classes

Ö's [built-in scope] contains things like basic data structures and commonly
needed functions. They are listed here.


## Functions

### debug

`debug x;` does the same thing as `print (x.to_debug_string);`. See
[Object](#object) for more info about the `to_debug_string` method.

### func

`func "name arg1 arg2 arg3" block;` defines a new function with arguments named
`arg1`, `arg2` and `arg3`.

The string argument of `func` is first split with the `split_by_whitespace`
string method; see [String's method documentation](#string). Everything except
the first part of the split result are argument names, and there may be 0 or
more of them. The resulting [Function object](#function) is then set to
`block`'s [definition scope] with the name from the string.

Argument names ending with `:` are option names, so `thing:` defines an option
named `thing`. The option may be specified when calling the function like
`the_function thing:"hello";`, and `the_function;` without specifying `thing`
is equivalent to `the_function thing:null;`. Option names must be after
argument names when defining the function, but not when calling the function;
`func "name arg1 opt: arg2" { ... };` throws [ValueError], but
`class "Someclass" inherits:AnotherClass { ... };` works.

When called, the function creates a new subscope of `block`'s
[definition scope], and inserts the values of the arguments there as local
variables. A `return` function that behaves like the `return` of
[Block](#block)'s `run_with_return` is also added, so `return foo;` inside the
function makes the function return `foo` and exits it.

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

I'm sorry, there's no way to do `else` yet :( If you need an else, do this:

```python
if something {
    ...
};
if (not something) {
    ...
};
```

### not

`(not x)` returns [false] if `x` is [true], or [true] if `x` is [false].

If `x` is not [true] or [false], `(not x)` throws [TypeError].

### equals

``(x `equals` y)`` returns [true] if `x` and `y` should be considered equal,
and [false] otherwise.

Right now Ö has only these rules for comparing equality of objects:
- If `x` and `y` are [the same object](#same-object), they compare equal.
- If `x` and `y` are [Strings](#string), their contents are compared.
- If `x` and `y` are [Integers](#integer), their values are compared.
- If `x` and `y` are [Arrays](#array), they compare equal if they have the same
  number of elements and the elements of `x` and `y` compare equal. Order
  matters, so `[1 2]` is not equal to `[2 1]`.
- If `x` and `y` are [Mappings](#mapping), the keys and values are checked for
  equality similarly to arrays.
- Otherwise, `x` and `y` are considered not equal.

Bugs:
- There's no way to override the default `equals` behaviour in custom classes.
- There really should be a `==` operator. Even though infix notation isn't too
  bad, `(x == y)` would be more readable than ``(x `equals` y)``.

### same_object

``(x `same_object` y)`` returns [true] if `x` and `y` point to the same object,
and [false] otherwise.

For example, this sets `x` and `y` to the same empty [array](#array)...

```python
var x = [];
var y = x;
```

...so you get behaviour like this:
- `x` and `y` are both empty, so ``(x `equals` y)`` returns [true]; see
  [equals](#equals).
- ``(x `same_object` y)`` returns [true].
- If you do `x.push "hi";` and then `print (y.to_debug_string)`, you'll get
  `["hi"]`. The `x` and `y` variables point to the same [array](#array), so
  doing something with `x` or `y` does that something to the array, regardless
  of the variable used.

But this creates two different objects...

```python
var x = [];
var y = [];
```

...and things behave a bit differently:
- `x` and `y` are both empty, so ``(x `equals` y)`` still returns [true].
- This time, ``(x `same_object` y)`` returns [false]!
- Doing something to `x` doesn't do the same thing to `y`.

### while

`while { return condition; } block;` runs `block` repeatedly until `condition`
evaluates to [false].

`while` creates a new subscope of `block`'s [definition scope]. Then it runs
the first argument in it, and that should return [true] or [false]; if it
returns something else, a `TypeError` is raised. If [true] was returned,
`block` is ran in the same subscope. Then the first argument is ran in the
subscope again and the process is repeated until the interpreter's stack gets
full or `condition` evaulates to [false].

Bugs:
- There's no `break` or `continue` yet.
- `while` is implemented with recursion, so infinite loops don't work.
- `while` is slow.
- The `{ return condition; }` syntax sucks. I'll hopefully implement a
  `{ condition }` syntax that is equivalent to `{ return condition; }` soon.

Example:

```python
# print numbers from 0 to 9
var i = 0;
while { return (not (i `equals` 10)); } {
    print (i.to_string);
    i = (i.plus 1);   # yes, this sucks... no + operator yet
};
```

### foreach

`foreach varname array block;` runs `block` once for each element of `array`.

First a new subscope of `block`'s [definition scope] is created. Then, for each
element of the array, a local variable named with the `varname` string is set
to the element in the subscope, and `block` is ran in the subscope.

Bugs:
- This is implemented with [while](#while), so it has many of the same bugs: no
  `break` or `continue`, segfaults the interpreter with long lists and is slow.
- I think `foreach` should be a method of [Array](#array) objects, not a
  function.

Example:

```python
# print "a", "b" and "c"
foreach "character" ["a" "b" "c"] {
    print character;
};
```

### catch and throw

See [the documentation about errors](errors.md).

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
[all values are Objects](#classes). Use [is_instance_of](#is_instance_of)
instead.

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
- `some_class.baseclass` is the class that `some_class` inherits from (see
  the [*] note below). `Object.baseclass` is [null](#null).
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
- `(some_class.to_debug_string)` returns a string like `<Class "the name">`
  where `the name` is `some_class.name`. See also [Object](#object)'s
  `to_debug_string` documentation.

Usually new classes are created with [class](#class), but you can also create
them with `(new Class name baseclass)`, where `Class` is `(get_class String)`
(see above), `name` will become the `.name` attribute and `baseclass` is
another `Class` object to inherit from (usually [Object](#object)). Pass
`inheritable:true` if you want to make the class inheritable, i.e. allow
creating more classes with the new class as baseclass. `new Class` throws an
error if `baseclass` is not inheritable.

### Object

This is the baseclass of all other classes; that is, all other classes inherit
from this class and thus have the methods that this class has.

New objects can be created with [new](#new) like `(new Object)`, but that's not
very useful because `Object` is meant to be used mostly as a base class for
other things. However, ``(x `is_instance_of` Object)`` always returns [true]
regardless of the type of `x`, and that could be useful with e.g. something
that checks types with [is_instance_of](#is_instance_of).

Methods:
- `(object.to_debug_string)` should return a programmer-friendly string that
  describes the object. [The debug function](#debug) uses this. `Object`'s
  `to_debug_string` returns a string like `<ClassName at 0xblablabla>` where
  `ClassName` is the name of the object's class and the `0xblablabla` part is
  different for each object.

Many objects also have a `to_string` method that should return a human-readable
string, but not all objects have a human-readable string representations, even
though all objects *must* have a programmer-readable string representation.

### Bool

There are two `Bool` objects, `true` and `false`. Creating more Bools wouldn't
make sense, so `(new Bool)` always throws an error. This means that
``(x `is_instance_of` Bool)`` means that either ``(x `same_object` true)`` or
``(x `same_object` false)``.

Methods:
- `(true.to_debug_string)` returns `"true"` and `(false.to_debug_string)`
  returns `"false"`. This overrides [Object](#object)'s `to_debug_string`.

### String

New strings cannot be created with `(new String)`; use *string literals* like
`"hello"` instead. There are more details about string literals
[here][string literals].

Strings are immutable, so they cannot be changed after creating the string;
there is no [array](#array)-like `set` method even though there's a `get`
method. Create a new string instead of modifying existing strings.

Strings behave a lot like [arrays](#array) of strings of length 1. For example,
`(["h" "e" "l" "l" "o"].get 3)` and `("hello".get 3)` both return `"l"`.

Methods:
- `string.get` is like the array `.get` method.
- `string.slice` is like the array `.get` method, but it returns a string
  instead of an array of characters.
- `(string1.concat string2)` returns the strings added together. For example,
  `("hello".concat "world")` returns `"helloworld"`.
- `(string.split_by_whitespace)` splits the string into an array of substrings
  separated by one or more Unicode whitespace characters (e.g. spaces, tabs or
  newlines). For example, `(" hello world test ".split_by_whitespace)` returns
  `["hello" "world" "test"]`.
- `(string.to_string)` returns the `string` unchanged. This is for consistency
  with the `to_string` methods of other classes; `to_string` is supposed to
  return a human-readable string representing the object, if any, and the
  human-readable string representing a string is the string itself.
- `(string.to_debug_string)` returns the string with quotes around it. This
  overrides [Object](#object)'s `to_debug_string`.

Missing features:
- There's no `get_length` method.
- There's no way to concatenate strings; `"hello" + "world"` doesn't work
  because there's no `+` operator yet.

### Integer

New integers can be created with integer literals like `123`, or with a
`(new Integer)` call like `(new Integer "123")`. The `new` call allows leading
zeros so that `(new Integer "-00000000012")` returns `-12`, but leading zeros
are not allowed in integer literals; see [this thing][integer literals] for
more details about integer literals.

Integers are immutable; see
[String](#string) for more details about what this means practically.

The smallest allowed value of an integer is
-(2<sup>63</sup>) = -9223372036854775808, and the biggest value is
2<sup>63</sup> - 1 = 9223372036854775807. I'm planning on implementing
`Integer` differently so that it won't have these restrictions.

Methods (`x` and `y` are Integers):
- `(x.plus y)` returns x+y. Yes, I know, Ö really needs a `+` operator...
- `(x.to_string)` converts the integer to a string without leading zeros.
- `(x.to_debug_string)` returns `(x.to_string)`. This overrides
  [Object](#object)'s `to_debug_string`.

Missing features:
- There's no support for other bases than 10. I think there's no need to add
  special syntax (`0xff` is useful in low-level languages like C, but confuses
  people in high-level languages), but syntax like `(new Integer "ff" 16)`
  would be useful for e.g. dealing with hexadecimal colors. Similarly,
  `(255.to_string 16)` could return `"ff"`.
- `.plus` is the only way to do arithmetic with integers right now. It really
  sucks!

### Array

Arrays represent ordered lists of elements.

New arrays cannot be created with `(new Array)`. Use `[` and `]` instead:

```python
var array = [1 2 3];
```

`[]` creates an empty array.

There are no size limits; you can add as many elements to an array as you want
(as long as the array fits in the computer's memory).

Attributes:
- `array.length` is the number of elements in the array as an
  [Integer](#integer).

Methods:
- `array.push item;` adds `item` to the end of the array.
- `(array.pop)` deletes and returns the last item from the end of the array.
- `(array.get i)` returns the `i`'th element from the array. `(array.get 0)`
  is the first element, `(array.get 1)` is the second and so on. An error is
  thrown if `i` is negative or not less than `(array.get_length)`.
- `array.set i value;` sets the `i`'th item of the array to `value`. See `get`
  for more information about the `i` index.
- `(array.slice i j)` returns a new array that contains the elements between
  `i` and `j`. Both `i` and `j` are interpreted so that `0` means the beginning
  of the array and `(thing.get_length)` means the end of the array. Too big or
  small indexes are "rounded" to the closest possible values.
- `(array.slice i)` returns `(array.slice i (array.get_length))`.
- `(array.to_debug_string)` calls `to_debug_string` methods of the array
  elements and returns the debug strings in square brackets, joined with
  spaces. This overrides [Object](#object)'s `to_debug_string`.

Annoyances:
- There's no `array.foreach` method. Use [the foreach function](#foreach)
  instead.
- There's no `array.map` method or a built-in `map` function, but you can
  easily define a `map` function like this:

    ```python
    func "map function array" {
        var mapped = [];
        foreach "item" array {
            mapped.push (function item);
        };
        return mapped;
    };
    ```

### Mapping

Mapping objects *map* keys to values; that is, looking up the value
corresponding to a key is easy and fast.

New Mappings can be created in a few different ways:
1. `(new Mapping)` creates an empty mapping.
2. `(new Mapping pair_array)` takes an array of `[key value]` pair arrays and
   adds the keys and values to the mapping.
3. [Options][options] can be used together with 1. or 2. to add values with
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
- `(mapping.get key)` returns the value of a key or throws an error if the key
  is not found.
- `mapping.delete key;` deletes a key-value pair from the mapping. An error is
  thrown if the key is not found.
- `(mapping.get_and_delete key)` is like `.delete`, but it returns the value
  of the deleted key.

Missing features:
- There's no way to loop over all the items in the mapping.

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
- `(block.run_with_return scope)` inserts a `return` function to the scope's
  local variables and runs the scope as with `block.run scope;`. The `return`
  function takes 1 or 0 arguments, throws a [MarkerError], and running the
  block terminates. `run_with_return` then catches the `MarkerError` and
  returns the value passed to the `return` function while running. If `return`
  is not called at all or it's called with no arguments, `null` is returned.

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
- `scope.parent_scope` is the scope that non-local variables are looked up from
  (see below), or [null](#null) if `scope` is the built-in scope.

For example, you can access the built-in scope like this:

```python
var builtin_scope = {}.definition_scope;
while { keep_going = (not (builtin_scope.definition_scope `same_object` null)); } {
    builtin_scope = builtin_scope.parent_scope;
};
# now (builtin_scope.local_vars.get "while") works
```

Methods:
- `scope.set_var varname value;` sets the value of an existing variable in the
  scope, or if it's not found, in the parent scope or its parent scope and so
  on. `{ x = 123; }.run scope;` without `var` in front of `x` is equivalent to
  `scope.set_var "x" 123;`.
- `(scope.get_var varname)` looks up a variable similarly to `set_var`, but
  returns it instead of changing its value. `{ print x; }.run scope;` is
  equivalent to `print (scope.get_var "x");`

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

Methods:
- `(function.partial arg1 arg2 arg3)` returns a new function that calls the
  original `function` with `arg1 arg2 arg3` and any arguments passed to the new
  function as arguments. So, `var g = (f.partial a b); g x y;` runs
  `f a b x y;`.
- `(function.to_debug_string)` returns a string like
  `<Function "the name of the function is here" at 0xblablabla>`. See also the
  `.name` attribute above. This overrides [Object](#object)'s `to_debug_string`.


## Other objects in the built-in namespace

These objects are not functions or classes.

### null

A dummy value like e.g. `none` or `nil` in many other programming languages.

The only instance of null's class is `null`. The class has a name `Null`, and
it has no methods or other attributes. You can access the class with
`(get_class null)` if you need that for some reason.


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
[options]: tutorial.md#options

[ArgError]: errors.md
[TypeError]: errors.md
[ValueError]: errors.md
[MarkerError]: errors.md
