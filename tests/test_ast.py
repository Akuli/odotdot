import pytest

from simplelang import tokenizer, ast_tree
from simplelang.ast_tree import (
    Array, String, Integer, Call, Block,
    SetVar, SetAttr, GetVar, GetAttr, CreateVar)


def parse(code):
    # list() calls are for fail-fast debuggability
    tokens = list(tokenizer.tokenize(code))
    return list(ast_tree.parse(tokens))


def parse_expression(code):
    result = parse('wat ' + code + ';')   # lol
    assert len(result) == 1
    assert isinstance(result[0], Call)
    assert result[0].func == GetVar('wat')
    assert len(result[0].args) == 1
    return result[0].args[0]


def test_integers():
    assert parse_expression('2') == Integer(2)
    assert parse_expression('-2') == Integer(-2)
    assert parse_expression('0') == parse_expression('-0') == Integer(0)


def test_strings():
    # TODO: allow \n, \t and stuff
    assert parse_expression('"hello"') == String('hello')


def test_arrays():
    assert parse_expression('[ ]') == Array([])
    assert parse_expression('[1 -2]') == Array([Integer(1), Integer(-2)])
    assert parse_expression('[[1 2 ]3[ [4]] ]') == Array([
        Array([Integer(1), Integer(2)]),
        Integer(3),
        Array([Array([Integer(4)])]),
    ])


def test_blocks():
    assert parse_expression('{ return 123; }') == Block([
        Call(GetVar('return'), [Integer(123)])])
    assert parse_expression('{ return 123; }') == parse_expression('{ 123 }')
    assert parse_expression('{ }') == Block([])
    assert parse_expression('{ asd; toot = { }; }') == Block([
        Call(GetVar('asd'), []),
        SetVar('toot', Block([])),
    ])


def test_block_bug():
    # bug doesn't do anything
    assert parse_expression('{ asd; toot = { }; }') == Block([
        Call(GetVar('asd'), []),
        SetVar('toot', Block([])),
    ])

    # ...but this has broken everything
    assert parse_expression('{ toot = { }; }') == Block([
        SetVar('toot', Block([])),
    ])


def test_variables():
    assert parse('var x=y;') == [CreateVar('x', GetVar('y'))]
    assert parse('var x;') == [CreateVar('x', GetVar('null'))]
    assert parse('x = y;') == [SetVar('x', GetVar('y'))]

    # invalid variable names
    with pytest.raises(AssertionError):
        parse('var var;')
    with pytest.raises(AssertionError):
        parse('var x.y;')

    # not valid expression
    with pytest.raises(AssertionError):
        parse('const x = ;')

    with pytest.raises(AssertionError):
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
    with pytest.raises(ValueError):
        parse('x.var.y;')
