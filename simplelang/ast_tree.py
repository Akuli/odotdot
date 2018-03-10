from collections import deque, namedtuple


# expressions
String = namedtuple('String', ['python_string'])
GetVar = namedtuple('GetVar', ['varname'])
GetAttr = namedtuple('GetAttr', ['object', 'attribute'])

# statements
Call = namedtuple('Call', ['func', 'args'])
SetVar = namedtuple('SetVar', ['varname', 'value'])
SetAttr = namedtuple('SetAttr', ['object', 'attribute', 'value'])
CreateVar = namedtuple('CreateVar', ['varname', 'value', 'is_const'])
CreateAttr = namedtuple('CreateAttr',
                        ['object', 'attribute', 'value', 'is_const'])


# this kind of abuses EOFError... feels nice and evil >:D MUHAHAHAA!!!
class _HandyTokenIterator:

    def __init__(self, iterable):
        self._iterator = iter(iterable)
        self._coming_up_stack = deque()

        # this is only used in _Parser.parse_file()
        self.last_popped = None

    def pop(self):
        try:
            result = self._coming_up_stack.pop()
        except IndexError:
            try:
                result = next(self._iterator)
            except StopIteration:   # pragma: no cover
                # it's surprisingly hard to make this bit run, that's
                # why it's not tested
                raise EOFError

        self.last_popped = result
        return result

    def coming_up(self, n=1):
        while len(self._coming_up_stack) < n:
            try:
                # pop() doesn't work if there's something on _coming_up_stack
                self._coming_up_stack.appendleft(next(self._iterator))
            except StopIteration as e:
                raise EOFError from e
        return self._coming_up_stack[-n]

    def something_coming_up(self):
        try:
            self.coming_up()
            return True
        except EOFError:
            return False


class _Parser:

    def __init__(self, tokens):
        self.tokens = _HandyTokenIterator(tokens)

    def parse_string(self):
        # "hello world"
        # TODO: "hello \"world\" ${some code}"
        token = self.tokens.pop()
        assert token.kind == 'string', "expected a string, not " + token.kind
        return String(token.value[1:-1])

    def expression_coming_up(self):
        return self.tokens.coming_up().kind in ('string', 'identifier')

    def parse_expression(self):
        if self.tokens.coming_up().kind == 'string':
            # "hello"
            result = self.parse_string()
        elif self.tokens.coming_up().kind == 'identifier':
            result = GetVar(self.tokens.pop().value)
        else:
            raise ValueError("should be a string or a variable name, not '%s'"
                             % self.tokens.coming_up().value)

        # attributes
        while (self.tokens.coming_up().kind == 'op' and
               self.tokens.coming_up().value == '.'):
            self.tokens.pop()
            attribute = self.tokens.pop()
            if attribute.kind != 'identifier':
                raise ValueError("invalid attribute name '%s'"
                                 % attribute.value)
            result = GetAttr(result, attribute.value)

        return result

    # this takes the function as an already-parsed argument
    # this way parse_statement() knows when this should be called
    def parse_call(self, func):
        args = []
        while self.expression_coming_up():
            args.append(self.parse_expression())
        return Call(func, args)

    def parse_var_or_const(self):
        keyword = self.tokens.pop()
        assert keyword.kind == 'keyword' and keyword.value in ('var', 'const')
        is_const = (keyword.value == 'const')

        target = self.parse_expression()

        if (self.tokens.coming_up().kind == 'op' and
                self.tokens.coming_up().value == '='):
            self.tokens.pop()
            initial_value = self.parse_expression()
        else:
            if is_const:
                raise ValueError("must specify a value, like "
                                 "'const varname = value;'")
            initial_value = GetVar('null')      # TODO: something better?

        if isinstance(target, GetVar):
            return CreateVar(target.varname, initial_value, is_const)
        if isinstance(target, GetAttr):
            return CreateAttr(
                target.object, target.attribute, initial_value, is_const)
        raise ValueError(
            "the x of '%s x = y;' must be a variable name or an attribute"
            % keyword.value)

    # this takes the first expression as an already-parsed argument
    # this way parse_statement() knows when this should be called
    def parse_assignment(self, target):
        equal_sign = self.tokens.pop()
        assert equal_sign.kind == 'op'
        assert equal_sign.value == '='
        value = self.parse_expression()

        if isinstance(target, GetVar):
            return SetVar(target.varname, value)
        if isinstance(target, GetAttr):
            return SetAttr(target.object, target.attribute, value)
        raise ValueError(
            "the x of 'x = y;' must be a variable name or an attribute")

    def parse_statement(self):
        if (self.tokens.coming_up().kind == 'keyword' and
                self.tokens.coming_up().value in ('var', 'const')):
            statement = self.parse_var_or_const()
        else:
            start = self.parse_expression()
            if (self.tokens.coming_up().kind == 'op' and
                    self.tokens.coming_up().value == '='):
                statement = self.parse_assignment(start)
            else:
                statement = self.parse_call(start)

        semicolon = self.tokens.pop()
        assert semicolon.kind == 'op', repr(semicolon)
        assert semicolon.value == ';', repr(semicolon)

        return statement

    def parse_file(self):
        while self.tokens.something_coming_up():
            yield self.parse_statement()


def parse(tokens):
    return _Parser(tokens).parse_file()
