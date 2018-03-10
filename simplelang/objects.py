class Object:
    pass


null = None


class NullClass(Object):

    def __init__(self):
        if null is not None:    # null object already created
            raise ValueError("cannot create more NullClass objects, use the "
                             "existing null object instead")


null = NullClass()


class String(Object):

    def __init__(self, python_string):
        self.python_string = python_string


class BuiltinFunction(Object):

    def __init__(self, python_func):
        self.python_func = python_func

    def call(self, args):
        return self.python_func(*args)


def add_builtins(context):
    context.add_variable('null', null)
    context.add_variable('print', BuiltinFunction(lambda arg: print(arg.python_string)))
