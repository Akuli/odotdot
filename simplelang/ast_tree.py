import collections


# expressions
String = collections.namedtuple('String', ['python_string'])
GetVar = collections.namedtuple('GetVar', ['varname'])

# statements
SetVar = collections.namedtuple('SetVar', ['varname', 'value'])
Call = collections.namedtuple('Call', ['func', 'args'])
VarStatement = collections.namedtuple('VarStatement', ['varname', 'value'])
ConstStatement = collections.namedtuple('ConstStatement', ['varname', 'value'])


# this kind of abuses EOFError... feels nice and evil >:D MUHAHAHAA!!!
class _HandyTokenIterator:

    def __init__(self, iterable):
        self._iterator = iter(iterable)
        self._coming_up_stack = collections.deque()

        # this is only used in _Parser.parse_file()
        self.last_popped = None

    def pop(self):
        try:
            result = self._coming_up_stack.pop()
        except IndexError:
            try:
                result = next(self._iterator)
            except StopIteration:
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
        coming_up = self.tokens.coming_up()
        if self.tokens.coming_up().kind == 'string':
            # "hello"
            return self.parse_string()
        if coming_up.kind == 'identifier':
            return GetVar(self.tokens.pop().value)
        raise ValueError("should be a string or variable name, not %s"
                         % coming_up.kind)

    def parse_call(self):
        func = self.parse_expression()
        args = []
        while self.expression_coming_up():
            args.append(self.parse_expression())
        return Call(func, args)

    def _pop_varname(self):
        token = self.tokens.pop()
        if token.kind != 'identifier':
            raise ValueError("invalid variable name " + repr(token.value))
        return token

    def parse_setvar(self):
        varname = self._pop_varname()

        equal_sign = self.tokens.pop()
        assert equal_sign.kind == 'op', equal_sign
        assert equal_sign.value == '=', equal_sign

        value = self.parse_expression()
        return SetVar(varname.value, value)

    def parse_const(self):
        const = self.tokens.pop()
        assert const.kind == 'keyword' and const.value == 'const'
        varname, value = self.parse_setvar()
        return ConstStatement(varname, value)

    def parse_var_statement(self):
        var = self.tokens.pop()
        assert var.kind == 'keyword' and var.value == 'var'
        if (self.tokens.coming_up(2).kind == 'op' and
                self.tokens.coming_up(2).value == '='):
            varname, initial_value = self.parse_setvar()
        else:
            varname = self._pop_varname().value
            initial_value = GetVar('null')      # TODO: something better?
        return VarStatement(varname, initial_value)

    def parse_statement(self):
        # there should be always at least two tokens coming up, some token
        # and a semicolon
        coming_1st = self.tokens.coming_up()
        coming_2nd = self.tokens.coming_up(2)
        if coming_1st.kind == 'keyword' and coming_1st.value == 'const':
            statement = self.parse_const()
        elif coming_1st.kind == 'keyword' and coming_1st.value == 'var':
            statement = self.parse_var_statement()
        elif coming_2nd.kind == 'op' and coming_2nd.value == '=':
            statement = self.parse_setvar()
        else:
            statement = self.parse_call()

        semicolon = self.tokens.pop()
        assert semicolon.kind == 'op', repr(semicolon)
        assert semicolon.value == ';', repr(semicolon)

        return statement

    def parse_file(self):
        while self.tokens.something_coming_up():
            yield self.parse_statement()


def parse(tokens):
    return _Parser(tokens).parse_file()


if __name__ == '__main__':
    from importlib import import_module as i
    while True:
        code = input('>> ')
        try:
            tokens = list(i('simplelang.tokenizer').tokenize(code))
            ast_tree = list(parse(tokens))
            i('pprint').pprint(ast_tree)
        except ValueError as e:
            print(e)
        except Exception as e:
            i('traceback').print_exc()
