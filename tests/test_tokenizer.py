import pytest

from simplelang.tokenizer import Token, Location, tokenize


# this is lol
class Anything:
    def __eq__(self, other):
        return True


# like Token, but ignores locations
def token(kind, value):
    return Token(kind, value, Location(Anything(), Anything()))


def tokenizelist(code):
    return list(tokenize(code, '<test>'))


def test_basic_stuff():
    for code in ['var x=y.z;', 'var  x\n \t= y\n.z;']:
        assert tokenizelist(code) == [
            token('keyword', 'var'),
            token('identifier', 'x'),
            token('op', '='),
            token('identifier', 'y'),
            token('op', '.'),
            token('identifier', 'z'),
            token('op', ';'),
        ]

    assert tokenizelist('blah varasd"hi"toot ') == [
        token('identifier', 'blah'),
        token('identifier', 'varasd'),
        token('string', '"hi"'),
        token('identifier', 'toot'),
    ]

    assert tokenizelist('blah123 123blah var123 123var') == [
        token('identifier', 'blah123'),
        token('integer', '123'),
        token('identifier', 'blah'),
        token('identifier', 'var123'),
        token('integer', '123'),
        token('keyword', 'var'),
    ]

    assert tokenizelist('a{b(c[d]e)f}g') == [   # noqa
        token('identifier', 'a'),
        token('op', '{'),
            token('identifier', 'b'),
            token('op', '('),
                token('identifier', 'c'),
                token('op', '['),
                    token('identifier', 'd'),
                token('op', ']'),
                token('identifier', 'e'),
            token('op', ')'),
            token('identifier', 'f'),
        token('op', '}'),
        token('identifier', 'g'),
    ]

    with pytest.raises(ValueError):
        tokenizelist('"hello\nworld"')


def test_comments():
    for code in ['print "h#e#l#l#o";   #a comment',
                 'print "h#e#l#l#o";   # a comment\n',
                 '# a comment\nprint "h#e#l#l#o";']:
        assert tokenizelist(code) == [
            token('identifier', 'print'),
            token('string', '"h#e#l#l#o"'),
            token('op', ';'),
        ]
