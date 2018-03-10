import collections
import re

# https://docs.python.org/3/library/re.html
token_spec = [
    ('keyword', 'var|const'),
    ('identifier', r'[^\W\d]\w*'),
    ('op', r'[{}=;]'),
    ('string', '".*?"'),
    ('whitespace', r'\s+'),
    ('error', '.'),
]
token_regex = '|'.join(r'(?P<%s>%s)' % pair for pair in token_spec)
Token = collections.namedtuple('Token', ['kind', 'value'])


def tokenize(code, filename='<string>'):
    lineno = 1
    line_start = 0

    for match in re.finditer(token_regex, code):
        kind = match.lastgroup
        value = match.group(kind)

        if kind == 'error':
            column = match.start() - line_start
            raise ValueError("invalid syntax in file '%s', line %d, column %d"
                             % (filename, lineno, column))

        if kind == 'whitespace':
            if '\n' in value:
                lineno += value.count('\n')
                line_start = match.start() + value.rindex('\n') + len('\n')
        else:
            yield Token(kind, value)


if __name__ == '__main__':
    for token in tokenize('display "hello world";\n\nword = input "asd"'):
        print(token)
