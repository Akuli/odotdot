import pytest

from simplelang.objects import Namespace, Object, String, NullClass, null


def test_namespace():
    ns = Namespace('toot')

    with pytest.raises(ValueError):
        ns.get('a')
    with pytest.raises(ValueError):
        ns.set_where_defined('a', Object())
    # make sure it did nothing before raising an error
    with pytest.raises(ValueError):
        ns.get('a')

    ns.set_locally('a', Object())
    assert isinstance(ns.get('a'), Object)
    ns.set_locally('a', String("hello"))
    assert isinstance(ns.get('a'), String)


def test_namespace_nesting():
    parent = Namespace('toot')
    child = Namespace('toot', parent)

    parent.set_locally('a', null)
    assert child.get('a') is null

    b_value = String("hi")
    child.set_locally('b', b_value)
    assert child.get('b') is b_value
    with pytest.raises(ValueError):
        parent.get('b')

    child.set_where_defined('a', String('wat'))
    assert child.get('a').python_string == 'wat'
    assert parent.get('a').python_string == 'wat'


def test_strings():
    assert String("hello").python_string == "hello"
    assert isinstance(String("hello"), Object)
    with pytest.raises(ValueError):
        String("hello").attributes.set_locally('toot', Object())


def test_null():
    assert isinstance(null, NullClass)
    with pytest.raises(TypeError):
        NullClass()
    with pytest.raises(ValueError):
        null.attributes.set_locally('asd', Object())
