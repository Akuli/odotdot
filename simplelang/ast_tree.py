from collections import deque, namedtuple
import itertools


# expressions
String = namedtuple('String', ['python_string'])
Integer = namedtuple('Integer', ['python_int'])
Array = namedtuple('Array', ['elements'])
Block = namedtuple('Block', ['statements'])
GetVar = namedtuple('GetVar', ['varname'])
GetAttr = namedtuple('GetAttr', ['object', 'attribute'])

# statements
# TODO: should attributes and variables be treated differently? people
# are used to that
SetVar = namedtuple('SetVar', ['varname', 'value'])
SetAttr = namedtuple('SetAttr', ['object', 'attribute', 'value'])
CreateVar = namedtuple('CreateVar', ['varname', 'value'])
CreateAttr = namedtuple('CreateAttr', ['object', 'attribute', 'value'])

# expressions that are also statements
Call = namedtuple('Call', ['func', 'args'])


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

    def parse_integer(self):
        token = self.tokens.pop()
        assert token.kind == 'integer', "expected an integer, not " + token.kind
        return Integer(int(token.value))

    def parse_array(self):
        open_bracket = self.tokens.pop()
        assert open_bracket.kind == 'op', open_bracket
        assert open_bracket.value == '[', open_bracket

        elements = []
        while not (self.tokens.coming_up().kind == 'op' and
                   self.tokens.coming_up().value == ']'):
            elements.append(self.parse_expression())
        self.tokens.pop()    # the ]

        return Array(elements)

    # remember to update this if you add more expressions!
    def expression_coming_up(self):
        if self.tokens.coming_up().kind in ('string', 'integer', 'identifier'):
            return True
        if self.tokens.coming_up().kind == 'op':
            return (self.tokens.coming_up().value in {'(', '{', '['})
        return False

    def parse_expression(self):
        if self.tokens.coming_up().kind == 'string':
            result = self.parse_string()
        elif self.tokens.coming_up().kind == 'integer':
            result = self.parse_integer()
        elif self.tokens.coming_up().kind == 'identifier':
            result = GetVar(self.tokens.pop().value)
        elif (self.tokens.coming_up().kind == 'op' and
              self.tokens.coming_up().value == '{'):
            result = self.parse_block()
        elif (self.tokens.coming_up().kind == 'op' and
              self.tokens.coming_up().value == '('):
            result = self.parse_call_expression()
        elif (self.tokens.coming_up().kind == 'op' and
              self.tokens.coming_up().value == '['):
            result = self.parse_array()
        else:
            raise ValueError('should be a variable name, "..." or { ... }, '
                             "not '%s'" % self.tokens.coming_up().value)

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

    def parse_call_expression(self):
        open_paren = self.tokens.pop()
        assert open_paren.kind == 'op'
        assert open_paren.value == '('

        func = self.parse_expression()
        result = self.parse_call_statement(func)    # see below

        close_paren = self.tokens.pop()
        assert close_paren.kind == 'op'
        assert close_paren.value == ')'

        return result

    # this takes the function as an already-parsed argument
    # this way parse_statement() knows when this should be called
    def parse_call_statement(self, func):
        args = []
        while self.expression_coming_up():
            args.append(self.parse_expression())
        return Call(func, args)

    def parse_var(self):
        var = self.tokens.pop()
        assert var.kind == 'keyword', keyword
        assert var.value == 'var', keyword

        target = self.parse_expression()

        if (self.tokens.coming_up().kind == 'op' and
                self.tokens.coming_up().value == '='):
            self.tokens.pop()
            initial_value = self.parse_expression()
        else:
            # TODO: something better for looking up builtins?
            initial_value = GetVar('null')

        if isinstance(target, GetVar):
            return CreateVar(target.varname, initial_value)
        if isinstance(target, GetAttr):
            return CreateAttr(
                target.object, target.attribute, initial_value)
        raise ValueError(
            "the x of 'var x = y;' must be a variable name or an attribute")

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
                self.tokens.coming_up().value == 'var'):
            statement = self.parse_var()
        else:
            start = self.parse_expression()
            if (self.tokens.coming_up().kind == 'op' and
                    self.tokens.coming_up().value == '='):
                statement = self.parse_assignment(start)
            else:
                statement = self.parse_call_statement(start)

        semicolon = self.tokens.pop()
        assert semicolon.kind == 'op', repr(semicolon)
        assert semicolon.value == ';', repr(semicolon)

        return statement

    def parse_block(self):
        open_brace = self.tokens.pop()
        assert open_brace.kind == 'op', repr(brace)
        assert open_brace.value == '{', repr(brace)

        # figure out whether it contains a semicolon before }
        # if it doesn't, it's an implicit return
        # { asd } is equivalent to { return asd; }
        if (self.tokens.coming_up().kind == 'op' and
                self.tokens.coming_up().value == '}'):
            # empty block: { }
            implicit_return = False
        else:
            # try to figure out if there's a semicolon before terminating }
            # must match braces because of nested code blocks (old bug)
            brace_counter = 1
            implicit_return = None
            for future_token in map(self.tokens.coming_up, itertools.count(1)):
                if future_token.kind == 'op':
                    if future_token.value == ';' and brace_counter == 1:
                        implicit_return = False
                        break
                    if future_token.value == '{':
                        brace_counter += 1
                    if future_token.value == '}':
                        brace_counter -= 1
                        if brace_counter == 0:
                            implicit_return = True
                            break

            if implicit_return is None:
                raise EOFError

        if implicit_return:
            statements = [Call(GetVar('return'), [self.parse_expression()])]
        else:
            statements = []
            while not (self.tokens.coming_up().kind == 'op' and
                       self.tokens.coming_up().value == '}'):
                statements.append(self.parse_statement())

        close_brace = self.tokens.pop()
        assert close_brace.kind == 'op', repr(close_brace)
        assert close_brace.value == '}', repr(close_brace)

        return Block(statements)

    def parse_file(self):
        while self.tokens.something_coming_up():
            yield self.parse_statement()


def parse(tokens):
    return _Parser(tokens).parse_file()
