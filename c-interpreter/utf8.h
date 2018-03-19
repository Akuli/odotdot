/* Convert a Unicode string to a UTF-8 string.

Return values:

- `0`: success
- `-1`: not enough memory
- `1`: the unicode string is invalid, an error message has been saved to `errormsg`

The error message is never more than 100 chars long (including the `\0`), so you
can pass an array of 100 chars to `errormsg`.
*/
int utf8_encode(unsigned long *unicode, size_t unicodelen, char **utf8, size_t *utf8len, char *errormsg);

/* Convert a UTF-8 string to a Unicode string.

Errors are handled similarly to [utf8_encode].
*/
int utf8_decode(char *utf8, size_t utf8len, unsigned long **unicode, size_t *unicodelen, char *errormsg);
