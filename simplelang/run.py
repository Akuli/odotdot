import collections
import functools

from simplelang import ast_tree, objects


class Context:

    def __init__(self, parent_context):
        self.parent_context = parent_context

        if parent_context is None:
            # global scope
            parent_namespaces = []
        else:
            parent_namespaces = parent_context._namespace.maps

        # self._namespace is {name: [value, is_constant]}
        # i know i know, tuples would be better... but they are immutable
        self._namespace = collections.ChainMap({}, *parent_namespaces)

    def add_variable(self, name, value, is_constant=True):
        if name in self._namespace:
            what = 'constant' if self._namespace[name][1] else 'variable'
            raise ValueError("there's already a %s named %s" % (what, name))

        self._namespace[name] = [value, is_constant]

    def set(self, varname, value):
        if varname not in self._namespace:
            raise ValueError("no variable named " + repr(varname))
        if self._namespace[varname][1]:
            raise ValueError("cannot set %s, it's a constant" % varname)
        self._namespace[varname][0] = value

    def get(self, varname):
        if varname not in self._namespace:
            raise ValueError("no variable named " + repr(varname))
        return self._namespace[varname][0]


builtin_context = Context(None)
objects.add_builtins(builtin_context)


class Interpreter:

    def __init__(self):
        self.global_context = Context(builtin_context)

    def evaluate(self, ast_expression, context):
        if isinstance(ast_expression, ast_tree.String):
            return objects.String(ast_expression.python_string)
        if isinstance(ast_expression, ast_tree.GetVar):
            return context.get(ast_expression.varname)

    def execute(self, ast_statements, context):
        evaluate = functools.partial(self.evaluate, context=context)

        for statement in ast_statements:
            if isinstance(statement, ast_tree.Call):
                func = self.evaluate(statement.func, context)
                args = [self.evaluate(arg, context) for arg in statement.args]
                func.call(args)
            elif isinstance(statement, (ast_tree.VarStatement,
                                        ast_tree.ConstStatement)):
                is_const = isinstance(statement, ast_tree.ConstStatement)
                value = self.evaluate(statement.value, context)
                context.add_variable(statement.varname, value, is_const)
            elif isinstance(statement, ast_tree.SetVar):
                value = self.evaluate(statement.value, context)
                context.set(statement.varname, value)
            else:
                raise RuntimeError("unknown statement: " + repr(statement))


if __name__ == '__main__':
    code = '''
    var msg;
    msg = "hello world";
    print msg;
    '''

    from simplelang import tokenizer
    tokens = tokenizer.tokenize(code)
    ast = list(ast_tree.parse(tokens))
    print(ast)
    interp = Interpreter()
    interp.execute(ast, interp.global_context)
