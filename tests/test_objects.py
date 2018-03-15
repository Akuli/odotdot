import pytest

#from simplelang.objects import Object, String, NullClass, null


#def test_strings():
#    assert String("hello").python_string == "hello"
#    assert isinstance(String("hello"), Object)
#    with pytest.raises(ValueError):
#        String("hello").attributes['toot'] = Object()
#
#
#def test_null():
#    assert isinstance(null, NullClass)
#    with pytest.raises(TypeError):
#        NullClass()
#    with pytest.raises(ValueError):
#        null.attributes.set_locally('asd', Object())
