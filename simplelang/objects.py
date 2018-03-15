import collections
import functools


# every simplelang class is represented with a ClassInfo object, not a
# python class
# generating new python classes "on the fly" wouldn't be too hard but
# this is simpler, and rewriting the interpreter in some
# not-as-dynamic-as-python programming language is easier this way
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


# this is the python type of all objects, subclassing this does not
# create a new simplelang class automagically
# again, the automagicness would be doable... but meh, kinda too much magic
#
# creating a new Object does NOT call setup! remember to do
# call_method('setup') separately
class Object:

    def __init__(self, class_info):
        self.class_info = class_info
        self.attributes = DefaultDictLikeThingy(self._get_method)

    def _get_method(self, name):
        try:
            func = self.class_info.methods[name]
        except KeyError:
            raise ValueError("no attribute named '%s'" % name)

        return new_function(functools.partial(func.python_func, self))

    # just for convenience
    def call_method(self, name, *args):
        return self.attributes[name].python_func(*args)

    # for debugging the python code
    def __repr__(self):
        return ('<simplelang object: %s>' %
                self.call_method('to_string').python_string)

    # __eq__ and __hash__ are for things like python's dict
    def __eq__(self, other):
        if not isinstance(other, Object):
            return NotImplemented
        return self.call_method('equals', other).python_bool

    def __hash__(self):
        return self.call_method('get_hash').python_int


# a helper function for python code, not exposed anywhere
# stdlib/fake_builtins.simple defines a pure-simplelang is_instance_of
def is_instance_of(obj, class_info):
    while class_info is not None:
        if class_info is obj.class_info:
            return True
        class_info = class_info.baseclass_info
    return False


# class info of the Object class that is the base class of everything
# setup, to_string, equals and get_hash are added to the methods dict later
object_info = ClassInfo(None, {})

# setup and to_string will be added later
function_info = ClassInfo(object_info, {})


def new_function(python_func):
    this = Object(function_info)
    this.python_func = python_func
    return this


def _error_raiser(message):
    def result(*args):
        raise ValueError(message)

    return new_function(result)


object_info.methods['setup'] = new_function(lambda this: null)
object_info.methods['to_string'] = new_function(
    lambda this: new_string('<an object at %#x>' % id(this)))
object_info.methods['equals'] = new_function(
    lambda this, that: true if this is that else false)
object_info.methods['get_hash'] = new_function(
    # object with lowercase o is python's object class
    lambda this: new_integer(object.__hash__(this)))

function_info.methods['setup'] = _error_raiser(
    "cannot create function objects directly, use 'func' instead")
function_info.methods['to_string'] = new_function(
    lambda this: new_string('<a function at %#x>' % id(this)))


null_class_info = ClassInfo(object_info, {
    'setup': _error_raiser("there's already a null object, use that instead "
                           "of creating a new null"),
})
null = Object(null_class_info)


def _string_equals(this, that):
    if not is_instance_of(that, string_info):
        return false
    return true if this.python_string == that.python_string else false


string_info = ClassInfo(object_info, {
    'setup': _error_raiser('cannot create new Strings directly, use string '
                           'literals like "hello" or "" instead'),
    'equals': new_function(_string_equals),
    'get_hash': new_function(
        lambda this: new_integer(hash(this.python_string))),
    'to_string': new_function(lambda this: this),
    'to_integer': new_function(
        lambda this: new_integer(int(this.python_string))),
    'to_array': new_function(
        lambda this: new_array(map(new_string, this.python_string))),
})


def new_string(python_string):
    this = Object(string_info)
    this.python_string = python_string
    return this


boolean_info = ClassInfo(object_info, {
    'setup': _error_raiser("use true and false instead of creating more "
                           "Booleans"),
    'to_string': new_function(
        lambda this: new_string('true' if this.python_bool else 'false')),
})

true = Object(boolean_info)
false = Object(boolean_info)
true.python_bool = True
false.python_bool = False


def new_integer(python_int):
    assert isinstance(python_int, int)
    this = Object(integer_info)
    this.python_int = python_int
    return this


def _integer_equals(this, that):
    if not is_instance_of(that, integer_info):
        return false
    return true if this.python_int == that.python_int else false


integer_info = ClassInfo(object_info, {
    'setup': _error_raiser("use integer literals like 0 or 123 instead of "
                           "'new new_integer'"),
    'equals': new_function(_integer_equals),
    'get_hash': new_function(lambda this: new_integer(hash(this.python_int))),
    'to_string': new_function(lambda this: new_string(str(this.python_int))),
})


def new_array(elements):
    this = Object(array_info)
    this.python_list = list(elements)
    return this


# this requires creating so many functions that i don't want to pollute
# the namespace...
def _create_array_info():
    def equals(this, that):
        if not is_instance_of(that, array_info):
            return false
        # python's == checks each element with __eq__, and __eq__ calls equals
        return true if this.python_list == that.python_list else false

    def foreach(this, varname, loop_body):
        assert is_instance_of(varname, string_info)
        assert is_instance_of(loop_body, block_info)

        context = (loop_body.attributes['definition_context']
                   .call_method('create_subcontext'))
        for element in this.python_list:
            context.local_vars[varname.python_string] = element
            loop_body.call_method('run', context)

    def slice(this, start, end=None):
        # python's thing[a:] is same as thing[a:None]
        the_end = None if end is None else end.python_int
        return new_array(this.python_list[start.python_int:the_end])

    def to_string(self):
        # TODO: what if i add the array inside itself? python does this:
        #    >>> a = [1]
        #    >>> a.append(a)
        #    >>> a
        #    [1, [...]]
        stringed = (element.call_method('to_string').python_string
                    for element in self.python_list)
        return new_string('[' + ' '.join(stringed) + ']')

    return ClassInfo(object_info, {
        'setup': _error_raiser("create new Arrays with square brackets, "
                               "like '[1 2 3]' or '[]'"),
        'equals': new_function(equals),
        'get_hash': _error_raiser("arrays are not hashable"),
        'add': new_function(lambda this, that: this.python_list.append(that)),
        # the copy works because new_array list()'s everything
        'copy': new_function(lambda this: new_array(this.python_list)),
        'foreach': new_function(foreach),
        'get': new_function(lambda this, i: this.python_list[i.python_int]),
        'slice': new_function(slice),
        'get_length': new_function(
            lambda this: new_integer(len(this.python_list))),
        'to_string': new_function(to_string),
    })


array_info = _create_array_info()


def _create_mapping_info():
    def setup(this, pairs):
        if not is_instance_of(pairs, array_info):
            raise TypeError("expected an array, got " +
                            pairs.call_method('to_string').python_string)

        this.python_dict = {}
        for item in pairs.python_list:
            if ((not is_instance_of(item, array_info)) or
                    item.call_method('get_length') != new_integer(2)):
                raise ValueError("expected an array of 2 elements, got " +
                                 item.call_method('to_string').python_string)

            key = item.call_method('get', new_integer(0))
            value = item.call_method('get', new_integer(1))
            this.python_dict[key] = value

    def equals(this, that):
        if not is_instance_of(that, mapping_info):
            return false
        return true if this.python_dict == that.python_dict else false

    def set_(this, key, value):
        this.python_dict[key] = value

    # python's dict.get returns None when default is not set and key not
    # found, this one raises an error instead
    def get(this, key, default=None):
        try:
            return this.python_dict[key]
        except KeyError as e:
            if default is None:
                raise e
            return default

    # TODO: a delete method?
    # TODO: iterators or views? keys? values?
    def items(self):
        return new_array(new_array(item) for item in self.python_dict.items())

    return ClassInfo(object_info, {
        'setup': new_function(setup),
        'equals': new_function(equals),
        'hash': _error_raiser("mappings are not hashable"),
        'set': new_function(set_),
        'get': new_function(get),
        'items': new_function(items),
    })


mapping_info = _create_mapping_info()


def new_block(interpreter, definition_context, ast_statements):
    assert ast_statements is not iter(ast_statements), (
        "ast_statements cannot be an iterator because it may need to be "
        "looped over several times")

    this = Object(block_info)
    this._interp = interpreter
    this._statements = ast_statements
    this.attributes['definition_context'] = definition_context
    return this


def _create_block_info():
    def run(this, context=null):
        if context is null:
            context = (this.attributes['definition_context']
                       .call_method('create_subcontext'))
        for statement in this._statements:
            this._interp.execute(statement, context)

    def run_with_return(this, context=null):
        try:
            this.call_method('run', context)
        except ReturnAValue as e:
            return e.value

    return ClassInfo(object_info, {
        'setup': _error_raiser('create new blocks with braces, '
                               'e.g. { print "hello"; }'),
        'run': new_function(run),
        'run_with_return': new_function(run_with_return),
    })


block_info = _create_block_info()


# class objects represent ClassInfos in the language
def _new_class_object(wrapped_info):
    this = Object(class_object_info)
    this.wrapped_class_info = wrapped_info
    if wrapped_info.baseclass_info is None:
        this.attributes['baseclass'] = null
    else:
        this.attributes['baseclass'] = (
            class_objects[wrapped_info.baseclass_info])
    return this


class_object_info = ClassInfo(object_info, {
    'setup': _error_raiser("use 'get_class' or 'class' instead of creating "
                           "new class objects directly"),
    'to_string': new_function(
        lambda this: new_string('<a class object at %#x>' % id(self))),
})
class_objects = DefaultDictLikeThingy(_new_class_object)


def get_class(obj):
    return class_objects[obj.class_info]


def new(class_object, *setup_args):
    this = Object(class_object.wrapped_class_info)
    this.call_method('setup', *setup_args)
    return this


# i know i know, you hate exceptions as control flow
# but this is simpler than polluting everything with checks for returns
# practicality beats purity
class ReturnAValue(Exception):

    def __init__(self, value):
        super().__init__(value)
        self.value = value


def return_(value=null):
    raise ReturnAValue(value)


# like apply in python 2
def call(func, arg_list):
    return func.call(arg_list.python_list)


# setup is added after creating the only two instances
def print_(arg):
    print(arg.call_method('to_string').python_string)


def if_(condition, body):
    if condition is true:
        body.call_method('run')
    elif condition is not false:
        raise ValueError("expected true or false, got " + repr(condition))


# create a function that takes an array of arguments
def array_func(func_body):
    def the_func(*args):
        run_context = (func_body.attributes['definition_context']
                       .call_method('create_subcontext'))
        run_context.local_vars['arguments'] = new_array(args)
        return func_body.call_method('run_with_return', run_context)

    return new_function(the_func)


def error(msg):
    raise ValueError(msg.python_string)     # lol


def same_object(a, b):
    return true if a is b else false


def equals(a, b):
    return a.call_method('equals', b)


# these suck
def setattr_(obj, name, value):
    obj.attributes.set_locally(name.python_string, value)


def getattr_(obj, name):
    return obj.attributes.get(name.python_string)


def add_real_builtins(context):
    context.local_vars['null'] = null
    context.local_vars['true'] = true
    context.local_vars['false'] = false
    context.local_vars['return'] = new_function(return_)
    context.local_vars['if'] = new_function(if_)
    context.local_vars['print'] = new_function(print_)
    context.local_vars['call'] = new_function(call)
    context.local_vars['setattr'] = new_function(setattr_)
    context.local_vars['getattr'] = new_function(getattr_)
    context.local_vars['array_func'] = new_function(array_func)
    context.local_vars['error'] = new_function(error)
    context.local_vars['equals'] = new_function(equals)
    context.local_vars['same_object'] = new_function(same_object)
    context.local_vars['new'] = new_function(new)
    context.local_vars['get_class'] = new_function(get_class)

    context.local_vars['Object'] = class_objects[object_info]
    context.local_vars['String'] = class_objects[string_info]
    context.local_vars['Boolean'] = class_objects[boolean_info]
    context.local_vars['Integer'] = class_objects[integer_info]
    context.local_vars['Array'] = class_objects[array_info]
    context.local_vars['Mapping'] = class_objects[mapping_info]
