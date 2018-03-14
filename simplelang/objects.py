import collections


# every simplelang class is represented with at most three kinds of python
# objects
#   - ClassInfo contains most things
#       * every object has a ClassInfo
#       * ClassInfo can't be an object because it would need to also have
#         a ClassInfo, we would get some kind of infinite recursion
#   - some types are associated with python classes that contain a ClassInfo
#       * when an object is created with 'new' from simplelang, the Python
#         class is NOT used
#       * types defined in simplelang are not associated with a python class
#         at all, just ClassInfo and maybe a ClassObject
#   - ClassObject is an Object that represents a type
#       * needed because ClassInfo can't be an object
#
# when a new object is created with the new function, it's always constructed
# with the Object python class, not any of the other python classes
class ClassInfo:

    # OMG OMG ITS A CLASS WITH JUST ONE METHOD KITTENS DIE
    def __init__(self, baseclass_info, methods):
        self.baseclass_info = baseclass_info    # None for Object

        # inherit methods
        if baseclass_info is None:
            self.methods = collections.ChainMap(methods)
        else:
            self.methods = collections.ChainMap(
                methods, *self.baseclass_info.methods.maps)


# like defaultdict, but the callback gets the key as an argument
class DefaultDictLikeThingy(dict):

    def __init__(self, missing_callback):
        super().__init__()
        self._callback = missing_callback

    def __missing__(self, key):
        result = self._callback(key)
        self[key] = result
        return result


class Object:
    # setup, to_string, equals and get_hash are added later
    class_info = ClassInfo(None, {})

    def __init__(self, setup_args, class_info=None):
        if class_info is None:
            # convenience for classes that represent simplelang types
            class_info = type(self).class_info

        self.class_info = class_info
        self.attributes = DefaultDictLikeThingy(self._get_method)

        # cannot use self.attributes['setup'] because that creates a new
        # function, and we get infinite recursion when setting up Functions
        try:
            setup = self.class_info.methods['setup']
        except KeyError:
            # Object.setup doesn't exist yet
            assert 'setup' not in Object.class_info.methods
        else:
            # cannot use .call() because it wants to return null, but null
            # doesn't exist yet when some objects are created
            setup.python_func(self, *setup_args)

    def _get_method(self, name):
        try:
            func = self.class_info.methods[name]
        except KeyError:
            raise ValueError("no attribute named '%s'" % name)

        def bound_method(*args):
            return func.call([self] + list(args))

        return Function(bound_method)

    # just for convenience
    def call_method(self, name, *args):
        return self.attributes[name].call(args)

    # for debugging the python code
    def __repr__(self):
        try:
            return ('<simplelang object: %s>' %
                    self.call_method('to_string').python_string)
        except KeyError:        # to_string is not created yet
            assert 'to_string' not in self.class_info.methods
            return super().__repr__()

    def __eq__(self, other):
        if not isinstance(other, Object):
            return NotImplemented

        try:
            return self.call_method('equals', other).python_bool
        except ValueError:
            # Object.equals doesn't exist yet
            assert 'equals' not in Object.class_info.methods
            return (self is other)

    def __hash__(self):
        try:
            return self.call_method('get_hash').python_int
        except ValueError:
            # Object.get_hash doesn't exist yet
            assert 'get_hash' not in Object.class_info.methods

            # object with a lowercase o is Python's object class
            return object.__hash__(self)


# a helper function for python code, not exposed anywhere
# stdlib/fake_builtins.simple defines a pure-simplelang is_instance_of
def is_instance_of(obj, class_info):
    while class_info is not None:
        if class_info is obj.class_info:
            return True
        class_info = class_info.baseclass_info
    return False


class Function(Object):
    # setup and to_string will be added later
    class_info = ClassInfo(Object.class_info, {})

    def __init__(self, python_func):
        self.python_func = python_func
        super().__init__([])

    def _setup(self, *args):
        if not hasattr(self, 'python_func'):   # above __init__ wasn't called
            raise ValueError("cannot create function objects directly, "
                             "use 'func' instead")

    def call(self, args):
        result = self.python_func(*args)
        if result is None:
            # this makes everything 10 times easier...
            result = null
        assert isinstance(result, Object), (
            "%r returned %r" % (self.python_func, result))
        return result


# true, false and Integer will be defined later
Object.class_info.methods['setup'] = Function(lambda this: None)
Function.class_info.methods['setup'] = Function(Function._setup)
Object.class_info.methods['equals'] = Function(
    lambda this, that: true if this is that else false)
Object.class_info.methods['get_hash'] = Function(
    # object with lowercase o is python's object class
    lambda this: Integer(object.__hash__(this)))


class String(Object):

    def __init__(self, python_string):
        self.python_string = python_string
        super().__init__([])

    def _setup(self, *args):
        if not hasattr(self, 'python_string'):   # above __init__ wasn't called
            raise ValueError(
                'cannot create new Strings directly, use string literals like '
                '"hello" or "" instead')

    def _equals(self, other):
        if not is_instance_of(self, String.class_info):
            return false
        return true if self.python_string == other.python_string else false

    def _hash(self):
        return Integer(hash(self.python_string))

    def _to_string(self):
        return self

    def _to_integer(self):
        return Integer(int(self.python_string))

    def _to_array(self):
        return Array(map(String, self.python_string))

    class_info = ClassInfo(Object.class_info, {
        'setup': Function(_setup),
        'equals': Function(_equals),
        'hash': Function(_hash),
        'to_string': Function(_to_string),
        'to_integer': Function(_to_integer),
        'to_array': Function(_to_array),
    })


Object.class_info.methods['to_string'] = Function(
    lambda this: String('<an object at %#x>' % id(this)))
Function.class_info.methods['to_string'] = Function(
    lambda this: String('<a function at %#x>' % id(this)))


class NullClass(Object):

    def __init__(self):
        self._marker = None
        super().__init__([])

    def _setup(self, *args):
        if not hasattr(self, '_marker'):
            raise ValueError("there's already a null object, use that instead "
                             "of creating a new null")

    def _to_string(self):
        return String('null')

    class_info = ClassInfo(Object.class_info, {
        'setup': Function(_setup),
        'to_string': Function(_to_string),
    })


null = NullClass()


# i know i know, you hate exceptions as control flow
# but this is simpler than polluting everything with checks for returns
# practicality beats purity
class ReturnAValue(Exception):

    def __init__(self, value):
        super().__init__(value)
        self.value = value


@Function
def return_(value=null):
    raise ReturnAValue(value)


class ClassObject(Object):

    def __init__(self, wrapped_class_info):
        assert isinstance(wrapped_class_info, ClassInfo)
        self.wrapped_class_info = wrapped_class_info
        super().__init__([])

    def _setup(self, *args):
        if not hasattr(self, 'wrapped_class_info'):
            raise ValueError(
                "use 'get_class' or 'class' instead of 'new Class'")

        if self.wrapped_class_info.baseclass_info is None:
            self.attributes['baseclass'] = null
        else:
            self.attributes['baseclass'] = (
                class_objects[self.wrapped_class_info.baseclass_info])

    def _to_string(self):
        return String('<a Class object at %#x>' % id(self))

    # TODO: more useful methods for type checking, accessing methods etc

    class_info = ClassInfo(Object.class_info, {
        'setup': Function(_setup),
        'to_string': Function(_to_string),
    })


class_objects = DefaultDictLikeThingy(ClassObject)


@Function
def get_class(obj):
    return class_objects[obj.class_info]


@Function
def new(class_object, *setup_args):
    return Object(setup_args, class_object.wrapped_class_info)


class Boolean(Object):

    def __init__(self, python_bool):
        self.python_bool = python_bool
        super().__init__([])

    def _setup(self, *args):
        if not hasattr(self, 'python_bool'):
            raise ValueError("there are already true and false objects, "
                             "use them instead of creating more Booleans")

    def _to_string(self):
        return String('true' if self.python_bool else 'false')

    class_info = ClassInfo(Object.class_info, {
        'setup': Function(_setup),
        'to_string': Function(_to_string),
    })


true = Boolean(True)
false = Boolean(False)


class Integer(Object):

    def __init__(self, python_int):
        self.python_int = python_int
        super().__init__([])

    def _setup(self, *args):
        if not hasattr(self, 'python_int'):
            raise ValueError("create new Integers with integer literals, "
                             "e.g. 123")

    def _equals(self, other):
        if not is_instance_of(other, Integer.class_info):
            return false
        return true if self.python_int == other.python_int else false

    class_info = ClassInfo(Object.class_info, {
        'setup': Function(_setup),
        'equals': Function(_equals),
        'get_hash': Function(lambda self: Integer(hash(self.python_int))),
        'to_string': Function(lambda self: String(str(self.python_int))),
    })


class Array(Object):

    def __init__(self, elements=()):
        self.python_list = list(elements)
        super().__init__([])

    def _setup(self, *args):
        if not hasattr(self, 'python_list'):
            raise ValueError("create new Arrays with square brackets, "
                             "like '[1 2 3]' or '[]'")

    def _equals(self, other):
        if not is_instance_of(other, Array.class_info):
            return false
        # python's == checks each element with __eq__
        return true if self.python_list == other.python_list else false

    def _get_hash(self):
        raise TypeError("arrays are not hashable")

    def _foreach(self, varname, loop_body):
        assert isinstance(varname, String)
        assert isinstance(loop_body, Block)

        context = (loop_body.attributes['definition_context']
                   .call_method('create_subcontext'))
        for element in self.python_list:
            context.local_vars[varname.python_string] = element
            loop_body.call_method('run', context)

    def _slice(self, start, end=None):
        # python's thing[a:] is same as thing[a:None]
        the_end = None if end is None else end.python_int
        return Array(self.python_list[start.python_int:the_end])

    def _to_string(self):
        # TODO: what if i add the array inside itself? python does this:
        #    >>> a = [1]
        #    >>> a.append(a)
        #    >>> a
        #    [1, [...]]
        stringed = (element.call_method('to_string').python_string
                    for element in self.python_list)
        return String('[' + ' '.join(stringed) + ']')

    class_info = ClassInfo(Object.class_info, {
        'setup': Function(_setup),
        'equals': Function(_equals),
        'get_hash': Function(_get_hash),
        'add': Function(lambda self, value: self.python_list.append(value)),
        # this copy method works because __init__ list()s
        'copy': Function(lambda self: Array(self.python_list)),
        'foreach': Function(_foreach),
        'get': Function(lambda self, i: self.python_list[i.python_int]),
        'slice': Function(_slice),
        'get_length': Function(lambda self: Integer(len(self.python_list))),
        'to_string': Function(_to_string),
    })


# like apply in python 2
@Function
def call(func, arg_list):
    return func.call(arg_list.python_list)


def _check_and_get_pairs(array):
    for item in array.python_list:
        if ((not isinstance(item, Array)) or
                item.call_method('get_length') != Integer(2)):
            raise ValueError("expected an array of 2 elements, got " +
                             item.call_method('to_string').python_string)
        yield (item.get(Integer(0)), item.get(Integer(1)))


class Mapping(Object):

    def __init__(self, elements=None):
        if elements is None:
            elements = Array()
        super().__init__([elements])

    def _setup(self, elements):
        self.python_dict = ({} if elements is None else dict(elements))
        self.attributes.read_only = True

    def _set(self, key, value):
        self.python_dict[key] = value

    # python's dict.get returns None when default is not set and key not
    # found, this one raisese instead
    def _get(self, key, default=None):
        try:
            return self.python_dict[key]
        except KeyError as e:
            if default is not None:
                return default
            raise e

    # TODO: a delete method?
    # TODO: iterators or views? keys? values?
    def _items(self):
        return Array(Array(item) for item in self.python_dict.items())

    class_info = ClassInfo(Object.class_info, {
        'setup': Function(_setup),
        'set': Function(_set),
        'get': Function(_get),
        'items': Function(_items),
    })


class Block(Object):

    def __init__(self, interpreter, definition_context, ast_statements):
        assert ast_statements is not iter(ast_statements), (
            "ast_statements cannot be an iterator because it may need to be "
            "looped over several times")

        self._interp = interpreter
        self._statements = ast_statements
        super().__init__([definition_context])

    def _setup(self, definition_context):
        if not hasattr(self, '_interp'):
            raise ValueError('create new blocks with braces, '
                             'e.g. { print "hello"; }')
        self.attributes['definition_context'] = definition_context

    def _run(self, context=null):
        if context is null:
            context = (self.attributes['definition_context']
                       .call_method('create_subcontext'))
        for statement in self._statements:
            self._interp.execute(statement, context)

    def _run_with_return(self, context=null):
        try:
            self.call_method('run', context)
        except ReturnAValue as e:
            return e.value

    class_info = ClassInfo(Object.class_info, {
        'setup': Function(_setup),
        'run': Function(_run),
        'run_with_return': Function(_run_with_return),
    })


@Function
def print_(arg):
    print(arg.call_method('to_string').python_string)


@Function
def if_(condition, body):
    if condition is true:
        body.call_method('run')
    elif condition is not false:
        raise ValueError("expected true or false, got " + repr(condition))


# create a function that takes an array of arguments
@Function
def array_func(func_body):
    @Function
    def the_func(*args):
        run_context = (func_body.attributes['definition_context']
                       .call_method('create_subcontext'))
        run_context.local_vars['arguments'] = Array(args)
        return func_body.call_method('run_with_return', run_context)

    return the_func


@Function
def error(msg):
    raise ValueError(msg.python_string)     # lol


@Function
def equals(a, b):
    return true if a == b else false


@Function
def same_object(a, b):
    return true if a is b else false


# these suck
@Function
def setattr_(obj, name, value):
    obj.attributes.set_locally(name.python_string, value)


@Function
def getattr_(obj, name):
    return obj.attributes.get(name.python_string)


def add_real_builtins(context):
    context.local_vars['null'] = null
    context.local_vars['true'] = true
    context.local_vars['false'] = false
    context.local_vars['return'] = return_
    context.local_vars['if'] = if_
    context.local_vars['print'] = print_
    context.local_vars['call'] = call
    context.local_vars['setattr'] = setattr_
    context.local_vars['getattr'] = getattr_
    context.local_vars['array_func'] = array_func
    context.local_vars['error'] = error
    context.local_vars['equals'] = equals
    context.local_vars['same_object'] = same_object
    context.local_vars['new'] = new
    context.local_vars['get_class'] = get_class

    context.local_vars['Class'] = class_objects[ClassObject.class_info]
    context.local_vars['Object'] = class_objects[Object.class_info]
    context.local_vars['String'] = class_objects[String.class_info]
    context.local_vars['Integer'] = class_objects[Integer.class_info]
    context.local_vars['Boolean'] = class_objects[Boolean.class_info]
    context.local_vars['Array'] = class_objects[Array.class_info]
    context.local_vars['Mapping'] = class_objects[Mapping.class_info]
