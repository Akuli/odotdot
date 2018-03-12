import pytest

from simplelang.tokenizer import Token, tokenize


def tokenizelist(code):
    return list(tokenize(code))


def test_basic_stuff():
    assert (tokenizelist('var x=y.z;') ==
            tokenizelist('var  x\n \t= y\n.z;') == [
        Token('keyword', 'var'),
        Token('identifier', 'x'),
        Token('op', '='),
        Token('identifier', 'y'),
        Token('op', '.'),
        Token('identifier', 'z'),
        Token('op', ';'),
    ])

    assert tokenizelist('blah varasd"hi"toot ') == [
        Token('identifier', 'blah'),
        Token('identifier', 'varasd'),
        Token('string', '"hi"'),
        Token('identifier', 'toot'),
    ]

    assert tokenizelist('blah123 123blah return123 123return') == [
        Token('identifier', 'blah123'),
        Token('integer', '123'),
        Token('identifier', 'blah'),
        Token('identifier', 'return123'),
        Token('integer', '123'),
        Token('keyword', 'return'),
    ]

    assert tokenizelist('a{b(c[d]e)f}g') == [   # noqa
        Token('identifier', 'a'),
        Token('op', '{'),
            Token('identifier', 'b'),
            Token('op', '('),
                Token('identifier', 'c'),
                Token('op', '['),
                    Token('identifier', 'd'),
                Token('op', ']'),
                Token('identifier', 'e'),
            Token('op', ')'),
            Token('identifier', 'f'),
        Token('op', '}'),
        Token('identifier', 'g'),
    ]

    with pytest.raises(ValueError):
        tokenizelist('"hello\nworld"')


def test_comments():
    for code in ['print "h#e#l#l#o";   #a comment',
                 'print "h#e#l#l#o";   # a comment\n',
                 '# a comment\nprint "h#e#l#l#o";']:
        assert tokenizelist(code) == [
            Token('identifier', 'print'),
            Token('string', '"h#e#l#l#o"'),
            Token('op', ';'),
        ]
