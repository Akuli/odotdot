import collections


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


class Object:

    def __init__(self):
        self.attributes = Namespace('attribute')


class NullClass(Object):

    def __init__(self):
        if 'null' in globals():    # null object already created
            raise TypeError("cannot create more NullClass objects, use the "
                            "existing null object instead")
        super().__init__()
        self.attributes.can_add = False


null = NullClass()


class String(Object):

    def __init__(self, python_string):
        super().__init__()
        self.python_string = python_string
        self.attributes.can_add = False


class BuiltinFunction(Object):

    def __init__(self, python_func):
        super().__init__()
        self.python_func = python_func
        self.attributes.can_add = False

    def call(self, args):
        return self.python_func(*args)


def add_builtins(namespace):
    namespace.add('null', null)
    namespace.add('print', BuiltinFunction(lambda arg: print(arg.python_string)))
