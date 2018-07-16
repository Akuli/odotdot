# encodings

**Note:** This documentation assumes that you are familiar with [ByteArray]
objects.

`<std>/encodings` contains functions for encoding and decoding text.
**Encoding** means converting a [String] object to a [ByteArray] (sorry,
[ByteArray] is not documented yet!), and **decoding** means converting a
[ByteArray] back to a string.

This is useful for passing strings to things that can only handle bytes. One
byte can have values between 0 and 255, but there are more than 255 different
kinds of characters, so simply using 1 byte value for each character is not a
good idea. Encodings are different ways to deal with this. For example, `"ö"`
encoded with the `utf-8` encoding is two bytes, `195 182`.


## Encoding class

The [get](#get) function returns `Encoding` objects, but you can also create an
encoding object yourself with `(new Encoding encode decode)` and [add](#add) it
if Ö doesn't know it by default.

`encode` and `decode` are attributes with functions as values, so they can be
used just like methods:
- `(encoding.encode string)` converts the [String] to a [ByteArray].
- `(encoding.decode bytearray)` converts the [ByteArray] to a [String].


## Functions for working with Encoding objects

### get

`(encodings.get name)` returns an `Encoding` object. The name is a string, and
it's treated case-insensitively, so `(encodings.get "UTF-8")` and
`(encodings.get "utf-8")` return the same object. [ValueError] is thrown if the
encoding is not found.

### add

After calling `encodings.add name encoding;` where `encoding` is an
[Encoding](#encoding-class) object, `(encodings.get name)` returns the
`encoding`. The `name` is treated case-insensitively. Added encodings can be
used the same way that other encodings can, so after
`encodings.add "lol" lol_encoding;` you can do `("hello".to_byte_array "lol")`.


## List of encodings

Ö comes with these encodings, but you can [add](#add) more.

| Names                 | Notes                                         | More info             |
| --------------------- | --------------------------------------------- | --------------------- |
| `"utf-8"`, `"utf8"`   | `.ö` files should be always read as UTF-8.    | [UTF-8 on Wikipedia]  |

[UTF-8 on Wikipedia]: https://en.wikipedia.org/wiki/UTF-8


[ValueError]: ../errors.md
[String]: ../builtins.md#string
[ByteArray]: ../builtins.md#bytearray
