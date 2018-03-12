import collections
import functools


# i know i know, you hate exceptions as control flow
# but this is simpler than polluting everything with checks for returns
# practicality beats purity
class ReturnAValue(Exception):

    def __init__(self, value):
        super().__init__(value)
        self.value = value


class Namespace:

    def __init__(self, content_string_for_messages, parent_namespace=None):
        # examples: 'attribute', 'variable'
        self.content_string_for_messages = content_string_for_messages
        self.parent_namespace = parent_namespace
        self.values = {}    # {name: value}

        # this can be set to False to prevent changing anything
        # TODO: better control???
        self.read_only = False

    # set a value for this namespace only, creating it if needed
    def set_locally(self, name, value):
        if self.read_only:
            raise ValueError(
                "cannot set %s" % self.content_string_for_messages)
        self.values[name] = value

    # figure out where a value is set, and change it there
    def set_where_defined(self, name, value):
        # cannot loop through self.values.maps because of read-onlyness
        if self.read_only:
            raise ValueError(
                "cannot set %s" % self.content_string_for_messages)

        if name in self.values:
            self.values[name] = value
        elif self.parent_namespace is not None:
            self.parent_namespace.set_where_defined(name, value)
        else:
            raise ValueError("no %s named '%s'"
                             % (self.content_string_for_messages, name))

    def get(self, name):
        try:
            return self.values[name]
        except KeyError as e:
            if self.parent_namespace is not None:
                return self.parent_namespace.get(name)
            raise ValueError("no %s named '%s'"
                             % (self.content_string_for_messages, name)) from e


def _get_attrs(obj):
    result = set()
    for attribute in obj.attributes.values:
        value = obj.attributes.get(attribute)
        # TODO: what if the value is not hashable?
        result.add((attribute, value))
    return result


class Object:

    def __init__(self):
        self.attributes = Namespace('attribute')

    # TODO: provide some way to customize these??
    def __eq__(self, other):
        if not isinstance(other, Object):
            return NotImplemented

        # TODO: is this tooo duck-typingy?
        return _get_attrs(self) == _get_attrs(other)

    def __hash__(self):
        return hash(frozenset(_get_attrs(self)))


def only_n_instances(n):
    def decorator(klass):
        current_number = 0
        old_init = klass.__init__

        @functools.wraps(old_init)
        def new_init(self, *args, **kwargs):
            nonlocal current_number
            if current_number >= n:
                raise TypeError("cannot create more than %d %s objects"
                                % (n, klass.__name__))
            current_number += 1
            old_init(self, *args, **kwargs)

        klass.__init__ = new_init
        return klass

    return decorator


@only_n_instances(1)
class NullClass(Object):

    def __init__(self):
        super().__init__()
        self.attributes.read_only = True

    def __eq__(self, other):
        if not isinstance(other, NullClass):
            return NotImplemented
        return (self is other)

    def __hash__(self):
        return hash(None)   # should be unique enough


null = NullClass()


@only_n_instances(2)
class Boolean(Object):

    def __init__(self, python_bool):
        super().__init__()
        self.python_bool = python_bool
        self.attributes.read_only = True

    def __eq__(self, other):
        if not isinstance(other, Boolean):
            return NotImplemented
        return self.python_bool == other.python_bool

    def __hash__(self):
        return hash(self.python_bool)


true = Boolean(True)
false = Boolean(False)


class String(Object):

    def __init__(self, python_string):
        super().__init__()
        self.python_string = python_string
        self.attributes.read_only = True

    def __repr__(self):
        return '<simplelang String object: %r>' % self.python_string

    # TODO: get rid of the boilerplate :(
    def __eq__(self, other):
        if not isinstance(other, String):
            return NotImplemented
        return self.python_string == other.python_string

    def __hash__(self):
        return hash(self.python_string)


class Integer(Object):

    def __init__(self, python_int):
        super().__init__()
        self.python_int = python_int
        self.attributes.read_only = True

    def __repr__(self):
        return '<simplelang Integer object: %r>' % self.python_int

    def __eq__(self, other):
        if not isinstance(other, Integer):
            return NotImplemented
        return self.python_int == other.python_int

    def __hash__(self):
        return hash(self.python_int)


class BuiltinFunction(Object):

    def __init__(self, python_func):
        super().__init__()
        self.python_func = python_func
        self.attributes.read_only = True

    def call(self, args):
        result = self.python_func(*args)
        assert isinstance(result, Object), (
            "%r returned %r" % (self.python_func, result))
        return result

    def __eq__(self, other):
        if not isinstance(other, BuiltinFunction):
            return NotImplemented
        return self.python_func == other.python_func

    def __hash__(self):
        return hash(self.python_func)


# to be expanded...
class Array(Object):

    def __init__(self, elements=()):
        super().__init__()
        self.python_list = list(elements)
        self.attributes.set_locally('foreach', BuiltinFunction(self.foreach))
        self.attributes.set_locally('get', BuiltinFunction(self._get))
        self.attributes.set_locally('slice', BuiltinFunction(self._slice))
        self.attributes.set_locally('copy', BuiltinFunction(self.copy))
        self.attributes.set_locally('add', BuiltinFunction(self.add))
        self.attributes.set_locally('get_length',
                                    BuiltinFunction(self._get_length))
        self.attributes.read_only = True

    def add(self, element):
        self.python_list.append(element)
        return null

    def copy(self):
        return Array(self.python_list)      # __init__ list()s

    def foreach(self, varname, loop_body):
        assert isinstance(varname, String)
        assert isinstance(loop_body, Block)

        context = loop_body.definition_context.create_subcontext()
        for element in self.python_list:
            context.namespace.set_locally(varname.python_string, element)
            loop_body.run(context)

        return null

    # there are no integer objects yet, everything is a string :(
    def _get(self, index):
        return self.python_list[index.python_int]

    def _slice(self, start, end=None):
        # python's thing[a:] is same as thing[a:None]
        return Array(self.python_list[start.python_int:end.python_int])

    def _get_length(self):
        return Integer(len(self.python_list))

    def __eq__(self, other):
        if not isinstance(other, Array):
            return NotImplemented
        return self.python_list == other.python_list

    # no __hash__


class Mapping(Object):

    def __init__(self, elements=None):
        super().__init__()
        self.python_dict = ({} if elements is None else dict(elements))

        self.attributes.set_locally(
            'set', BuiltinFunction(self.python_dict.__setitem__))
        self.attributes.set_locally('get', BuiltinFunction(self.get))
        self.attributes.set_locally(
            'delete', BuiltinFunction(self.python_dict.__delitem__))
        self.attributes.read_only = True

    @classmethod
    def from_pairs(cls, *pairs):
        assert len(pairs) % 2 == 0, "odd number of arguments: " + repr(pairs)

        # idiomatic python: iterate in pairs
        result = {}
        iterator = iter(pairs)
        for key, value in zip(iterator, iterator):
            result[key] = value

        return cls(result)

    def set(self, key, value):
        self.python_dict[key] = value

    # python's dict.get returns None when default is not set and key not
    # found, this one raisese instead
    def get(self, key, default=None):
        try:
            return self.python_dict[key]
        except KeyError as e:
            if default is not None:
                return default
            raise e

    # TODO: keys,values,items equivalents? maybe as iterators or views?


class Block(Object):

    def __init__(self, interpreter, definition_context, ast_statements):
        assert ast_statements is not iter(ast_statements), (
            "ast_statements cannot be an iterator because it may need to be "
            "looped over several times")

        super().__init__()
        self._interp = interpreter
        self.definition_context = definition_context
        self._statements = ast_statements

        self.attributes.set_locally('definition_context', definition_context)
        self.attributes.set_locally('run', BuiltinFunction(self.run))
        self.attributes.set_locally('run_with_return',
                                    BuiltinFunction(self.run_with_return))
        self.attributes.read_only = True

    def run(self, context=None):
        if context is None:
            context = self.definition_context.create_subcontext()
        for statement in self._statements:
            self._interp.execute(statement, context)
        return null

    def run_with_return(self, context=None):
        try:
            self.run(context)
        except ReturnAValue as e:
            return e.value
        return null


@BuiltinFunction
def print_(arg):
    if isinstance(arg, String):
        print(arg.python_string)
    elif isinstance(arg, Integer):
        print(arg.python_int)
    else:
        raise TypeError("cannot print " + repr(arg))
    return null


@BuiltinFunction
def if_(condition, body):
    if condition is true:
        body.run()
    elif condition is not false:
        raise ValueError("expected true or false, got " + repr(condition))
    return null


# create a function that takes an array of arguments
@BuiltinFunction
def array_func(func_body):
    @BuiltinFunction
    def the_func(*args):
        run_context = func_body.definition_context.create_subcontext()
        run_context.namespace.set_locally('arguments', Array(args))
        return func_body.run_with_return(run_context)

    return the_func


@BuiltinFunction
def error(msg):
    raise ValueError(msg.python_string)     # lol


@BuiltinFunction
def equals(a, b):
    return true if a == b else false


def add_real_builtins(namespace):
    namespace.set_locally('null', null)
    namespace.set_locally('true', true)
    namespace.set_locally('false', false)
    namespace.set_locally('if', if_)
    namespace.set_locally('print', print_)
    namespace.set_locally('array_func', array_func)
    namespace.set_locally('error', error)
    namespace.set_locally('equals', equals)
    namespace.set_locally('Mapping', BuiltinFunction(Mapping.from_pairs))
    namespace.set_locally('Object', BuiltinFunction(Object))
