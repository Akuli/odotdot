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

    assert tokenizelist('const y = "hello world";') == [
        Token('keyword', 'const'),
        Token('identifier', 'y'),
        Token('op', '='),
        Token('string', '"hello world"'),
        Token('op', ';'),
    ]

    with pytest.raises(ValueError):
        tokenizelist('"hello\nworld"')
