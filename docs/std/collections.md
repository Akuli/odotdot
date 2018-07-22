# collections

`<std>/collections` contains base classes that are useful for implementing
custom objects that can be used everywhere that e.g. [Array] can be used.


## Iterable

This is a base class for all objects that can be `foreach`-looped over. For
example, [Array] is a subclass of `Iterable`, so you can do this:

```python
[1 2 3].foreach "number" {
    print number.(to_string);
};
```

Methods:
- `iterable.foreach varname block;` runs `block` once for each element of
  `iterable`. First a new subscope of `block`'s [definition scope] is created,
  and for each element of the iterable, a local variable named `varname` is
  added to the subscope and the block is ran in the subscope. There's no
  `break` or `continue` yet :(
- `iterable.(get_iterator)` returns an [Iterator](#iterator) so that
  `iterable.(get_iterator).foreach` and `iterable.foreach` do the same thing.
  Subclasses of `iterable` must override this.


## Iterator

Foreach looping over an iterable is done with an iterator. The `Iterator` class
is a subclass of [Iterable](#iterable), but note that `Iterator`s can be looped
over only once; then they become *exhausted*, and looping over them more throws
an error.

Attributes:
- `iterator.is_exhausted` is `true` if the iterator is exhausted, and `false`
  otherwise. See `iterator.(next)` below for details.

Methods:
- `iterator.foreach` is like [Iterable](#iterable)'s `foreach`.
- `iterator.(get_iterator)` returns the `iterator`.
- `iterator.(next)` is called repeatedly in `iterator.foreach`, and it returns
  an [Option] of the next value. If there are no more values, `none` is
  returned and the iterator becomes exhausted. If the iterator is *already*
  exhausted when `iterator.(next)` is called, [ValueError] is thrown.

New iterators are created with `(new Iterator function)`. The `function` should
be a returning function, and it is called with no arguments in
`iterator.(next)`. The `function` should return an [Option] of the next element
like `iterator.(next)`, but it doesn't need to check if the iterator is
exhausted or make the iterator exhausted; `next` handles that.


## FrozenArrayLike

This subclass of [Iterable](#iterable) represents objects that behave like
[Array]s, but whose content can't be changed. For example, [String] is a
subclass of `FrozenArrayLike`. It has some of the methods that [Array] has,
like `get` and `foreach`, but methods like `push` and `pop` are missing because
they would change the content of the string.

Attributes that must be overrided when inheriting:
- `frozenarraylike.length` should return the number of elements in the
  array-like object as an [Integer].

Methods that must be overrided:
- `frozenarraylike.(get i)` should return the `i`'th element from the
  array-like object, where `0` is the first element, `1` is the second etc. A
  [ValueError] should be thrown if `i` is negative or not less than
  `frozenarraylike.length`.
- `frozenarraylike.(slice i j)` should return another `FrozenArrayLike` object
  that represents the items between `i` and `j`. The `i` and `j` work so that 0
  is the beginning and `frozenarraylike.length` is the end. For example,
  `frozenarraylike.(slice 2 4)` gives `frozenarraylike.(get 2)` and
  `frozenarraylike.(get 3)`. Out-of-bounds `i` and `j` values are interpreted
  as the closest possible correct values.


## ArrayLike

This class represents mutable objects like [Array]. It inherits
[FrozenArrayLike] because non-frozen array-like objects whose content can be
changed can do everything that frozen array-like objects can.

Subclasses must override all [FrozenArrayLike](#frozenarraylike) methods that
need overriding *and* these methods:
- `arraylike.push item;` should add an `item` to the end of the array.
- `arraylike.(pop)` should delete the last item from the array and return it.


## FrozenMappingLike

This class represents objects that behave like [Mapping], but whose
content can't be changed. This is *not* a subclass of
[Iterable](#iterable); mapping-like objects don't have a `foreach`
method in Ö; instead, this class inherits directly from [Object].

Attributes that need to be overrided:
- `frozenmappinglike.length` should be the number of key,value pairs in
  the mapping-like object.

Methods that need to be overrided:
- `frozenmappinglike.(get key)` should return the value that corresponds
  to `key`, or throw [KeyError] if the value is not found.

Methods that don't need to be overrided:
- `frozenmappinglike.(get_with_fallback key fallback)` returns
  `frozenmappinglike.(get key)`, but if the key is not found, it returns
  `fallback` instead of throwing [KeyError]. This is implemented by
  calling `frozenmappinglike.(get key)` and [catching] [KeyError].


## MappingLike

This is the baseclass of [Mapping] and a subclass of
[FrozenMappingLike](#frozenmappinglike). Inherit from this if you want
to create a mapping-like object that has methods for changing the
content, like `set` and `delete`.

Subclasses must override all attributes and methods of
[FrozenMappingLike](#frozenmappinglike) that need overriding *and* these
methods:
- `mappinglike.set key value;` should set the value of a key that is
  already in the mapping, or add a new key,value pair if the key is not
  found.
- `mappinglike.(get_and_delete key)` should delete a `key,value` pair
  from the mapping, looked up by the key.

Methods that don't need to be overrided:
- `mappinglike.delete key;` calls `mappinglike.(get_and_delete key)` and
  does nothing with the result.


[Array]: ../builtins.md#array
[Block]: ../builtins.md#block
[Integer]: ../builtins.md#integer
[Mapping]: ../builtins.md#mapping
[Object]: ../builtins.md#object
[Option]: ../builtins.md#option
[String]: ../builtins.md#string
[none]: ../builtins.md#none
[subscope]: ../tutorial.md#scopes
[definition scope]: ../tutorial.md#scopes
[catching]: ../errors.md#catching
[ValueError]: ../errors.md
[KeyError]: ../errors.md
