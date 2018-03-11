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

        if parent_namespace is None:
            parent_values = []
        else:
            parent_values = parent_namespace.values.maps

        # self.values is {name: [value, is_constant]}
        # i know i know, tuples would be better... but they are immutable
        # the {} means that anything defined here is not visible in the
        # parent namespaces
        self.values = collections.ChainMap({}, *parent_values)

        # this can be set to False to prevent add()ing more things
        self.can_add = True

    def add(self, name, initial_value, is_const=True):
        if not self.can_add:
            raise ValueError("cannot create more {}s"
                             .format(self.content_string_for_messages))
        if name in self.values:
            kind = 'constant' if self.values[name][1] else 'non-constant'
            raise ValueError("there's already a %s '%s' %s"
                             % (kind, name, self.content_string_for_messages))
        self.values[name] = [initial_value, is_const]

    def set(self, name, value):
        try:
            if self.values[name][1]:
                raise ValueError("cannot set '%s', it's a constant" % name)
            self.values[name][0] = value
        except KeyError as e:
            raise ValueError("no %s named '%s'"
                             % (self.content_string_for_messages, name)) from e

    def get(self, name):
        try:
            return self.values[name][0]
        except KeyError as e:
            raise ValueError("no %s named '%s'"
                             % (self.content_string_for_messages, name)) from e


def _get_attrs(obj):
    result = set()
    for attribute in obj.attributes.values:
        result.add((attribute, obj.attributes.get(attribute)))
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
        self.attributes.can_add = False

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
        self.attributes.can_add = False

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
        self.attributes.can_add = False

    def __repr__(self):
        return '<simplelang String object: %r>' % self.python_string

    # TODO: get rid of the boilerplate :(
    def __eq__(self, other):
        if not isinstance(other, String):
            return NotImplemented
        return self.python_string == other.python_string

    def __hash__(self):
        return hash(self.python_string)


class BuiltinFunction(Object):

    def __init__(self, python_func):
        super().__init__()
        self.python_func = python_func
        self.attributes.can_add = False

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


# a special marker object, get_context is a simplelang function that
# returns the current Context object
get_context = Object()


# to be expanded...
class Array(Object):

    def __init__(self, elements=()):
        super().__init__()
        self.python_list = list(elements)
        self.attributes.add('foreach', BuiltinFunction(self.foreach))
        self.attributes.can_add = False

    @classmethod
    def from_star_args(cls, *args):
        return cls(args)

    def foreach(self, varname, loop_body):
        assert isinstance(varname, String)
        assert isinstance(loop_body, Code)

        context = loop_body.definition_context.create_subcontext()
        iterator = iter(self.python_list)
        try:
            first = next(iterator)
        except StopIteration:
            return

        context.namespace.add(varname.python_string, first, is_const=False)
        loop_body.run(context)
        for not_first in iterator:
            context.namespace.set(varname.python_string, not_first)
            loop_body.run(context)

        return null


class Mapping(Object):

    def __init__(self, elements=None):
        super().__init__()
        self.python_dict = ({} if elements is None else dict(elements))

        self.attributes.add(
            'set', BuiltinFunction(self.python_dict.__setitem__))
        self.attributes.add('get', BuiltinFunction(self.get))
        self.attributes.add(
            'delete', BuiltinFunction(self.python_dict.__delitem__))
        self.attributes.can_add = False

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


class Code(Object):

    def __init__(self, interpreter, definition_context, ast_statements):
        assert ast_statements is not iter(ast_statements), (
            "ast_statements cannot be an iterator because it may need to be "
            "looped over several times")

        super().__init__()
        self._interp = interpreter
        self.definition_context = definition_context
        self._statements = ast_statements

        # TODO: add some kind of optional arguments :(
        self.attributes.add('run', BuiltinFunction(self.run))
        self.attributes.add('run_with_return',
                            BuiltinFunction(self.run_with_return))
        self.attributes.can_add = False

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
        raise ValueError("nothing was returned")


@BuiltinFunction
def print_(arg):
    assert isinstance(arg, String), "cannot print " + repr(arg)
    print(arg.python_string)
    return null


@BuiltinFunction
def if_(condition, code):
    assert isinstance(condition, Boolean), (
        "expected true or false, got " + repr(condition))
    if condition is true:
        code.run()
    return null


def add_builtins(namespace):
    namespace.add('null', null)
    namespace.add('true', true)
    namespace.add('false', false)
    namespace.add('if', if_)
    namespace.add('print', print_)
    namespace.add('get_context', get_context)
    namespace.add('Mapping', BuiltinFunction(Mapping.from_pairs))
    namespace.add('Object', BuiltinFunction(Object))
    namespace.add('Array', BuiltinFunction(Array.from_star_args))
