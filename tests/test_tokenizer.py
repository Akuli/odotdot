import pytest

from simplelang.tokenizer import Token, tokenize


def tokenizelist(code):
    return list(tokenize(code))


def test_basic_stuff():
    assert tokenizelist('var x=y;') == tokenizelist('var  x\n \t= y\n;') == [
        Token('keyword', 'var'),
        Token('identifier', 'x'),
        Token('op', '='),
        Token('identifier', 'y'),
        Token('op', ';'),
    ]

    assert tokenizelist('blah var asd"hi"toot ') == [
        Token('identifier', 'blah'),
        Token('keyword', 'var'),
        Token('identifier', 'asd'),
        Token('string', '"hi"'),
        Token('identifier', 'toot'),
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
