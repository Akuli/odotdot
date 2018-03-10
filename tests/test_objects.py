import pytest

from simplelang.objects import Namespace, Object, String, NullClass, null


def test_namespace():
    ns = Namespace('toot')

    with pytest.raises(ValueError):
        ns.get('a')
    with pytest.raises(ValueError):
        ns.set('a', Object())

    ns.add('a', Object())
    assert isinstance(ns.get('a'), Object)
    with pytest.raises(ValueError):     # it's a constant
        ns.set('a', String("hello"))

    # make sure it did nothing before raising an error
    assert not isinstance(ns.get('a'), String)

    # b is not a constant
    ns.add('b', Object(), False)
    assert isinstance(ns.get('b'), Object)
    ns.set('b', String("hello"))
    assert ns.get('b').python_string == "hello"

    with pytest.raises(ValueError):
        ns.add('a', Object())
    with pytest.raises(ValueError):
        ns.add('b', Object())


def test_namespace_nesting():
    parent = Namespace('toot')
    child = Namespace('toot', parent)

    parent.add('a', null)
    assert child.get('a') is null

    b_value = String("hi")
    child.add('b', b_value)
    assert child.get('b') is b_value
    with pytest.raises(ValueError):
        parent.get('b')


def test_strings():
    assert String("hello").python_string == "hello"
    assert isinstance(String("hello"), Object)
    with pytest.raises(ValueError):
        String("hello").attributes.add('toot', Object())


def test_null():
    assert isinstance(null, NullClass)
    with pytest.raises(TypeError):
        NullClass()
    with pytest.raises(ValueError):
        null.attributes.add('asd', Object())
