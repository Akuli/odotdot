import pytest

from simplelang import tokenizer
from simplelang.ast_tree import (
    parse, Call, SetVar, GetVar, ConstStatement, VarStatement, String)


def tokenize_parse(code):
    # list() calls are for fail-fast debuggability
    tokens = list(tokenizer.tokenize(code))
    return list(parse(tokens))


def test_strings():
    # TODO: allow \n, \t and stuff
    assert tokenize_parse('print "hello";') == [
        Call(GetVar('print'), [String('hello')]),
    ]


def test_vars():
    assert tokenize_parse('var x = y;') == [VarStatement('x', GetVar('y'))]
    assert tokenize_parse('const x = y;') == [ConstStatement('x', GetVar('y'))]
    assert tokenize_parse('var x;') == [VarStatement('x', GetVar('null'))]
    with pytest.raises(AssertionError):     # lol
        tokenize_parse('const x;')
    assert tokenize_parse('x = y;') == [SetVar('x', GetVar('y'))]

    # invalid variable names
    with pytest.raises(ValueError):
        tokenize_parse('var var;')
    with pytest.raises(ValueError):
        tokenize_parse('var const;')

    # not valid expression
    with pytest.raises(ValueError):
        tokenize_parse('const x = ;')
