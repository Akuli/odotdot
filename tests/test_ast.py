import pytest

from simplelang import tokenizer, ast_tree
from simplelang.ast_tree import (
    String, Call, SetVar, SetAttr, GetVar, GetAttr, CreateVar, CreateAttr)


def parse(code):
    # list() calls are for fail-fast debuggability
    tokens = list(tokenizer.tokenize(code))
    return list(ast_tree.parse(tokens))


def test_strings():
    # TODO: allow \n, \t and stuff
    assert parse('print "hello";') == [
        Call(GetVar('print'), [String('hello')]),
    ]


def test_variables():
    assert parse('var x=y;') == [CreateVar('x', GetVar('y'))]
    assert parse('var x;') == [CreateVar('x', GetVar('null'))]
    assert parse('x = y;') == [SetVar('x', GetVar('y'))]

    # invalid variable names
    with pytest.raises(ValueError):
        parse('var var;')

    # not valid expression
    with pytest.raises(AssertionError):
        parse('const x = ;')

    with pytest.raises(ValueError):
        parse('var "hello"=asd;')
    with pytest.raises(ValueError):
        parse('"hello"=asd;')


def test_attributes():
    assert parse('x.y;') == [Call(GetAttr(GetVar('x'), 'y'), [])]
    assert parse('a.b.c.d.e;') == [Call(
        GetAttr(GetAttr(GetAttr(GetAttr(GetVar('a'), 'b'), 'c'), 'd'), 'e'),
        [])]
    assert parse('a.b.c.d.e = f;') == [SetAttr(
        GetAttr(GetAttr(GetAttr(GetVar('a'), 'b'), 'c'), 'd'),
        'e', GetVar('f'))]
    assert parse('var x.y = z;') == [
        CreateAttr(GetVar('x'), 'y', GetVar('z'))]

    with pytest.raises(ValueError):
        parse('x.var.y;')
