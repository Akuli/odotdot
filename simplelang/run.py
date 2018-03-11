import collections
import functools
import os

from simplelang import tokenizer, ast_tree, objects


class Context(objects.Object):

    def __init__(self, parent_context):
        super().__init__()

        # this is the builtin context if parent_context is None
        self.parent_context = parent_context

        if parent_context is None:
            self.namespace = objects.Namespace('variable')
        else:
            self.namespace = objects.Namespace(
                'variable', parent_context.namespace)

        self.attributes.add('add_var', objects.BuiltinFunction(self._add_var))
        self.attributes.add('add_const',
                            objects.BuiltinFunction(self._add_const))
        self.attributes.add('set_var', objects.BuiltinFunction(self._set_var))
        self.attributes.add('get_var', objects.BuiltinFunction(self._get_var))

        if parent_context is None:
            self.attributes.add('parent_context', objects.null)
        else:
            self.attributes.add('parent_context', parent_context)
        self.attributes.add('create_subcontext',
                            objects.BuiltinFunction(self.create_subcontext))
        self.attributes.can_add = False

    # these are just for exposing in simplelang, access .namespace
    # directly in python instead of using this stupid poop
    def _add_var(self, name, initial_value):
        assert isinstance(name, objects.String)
        self.namespace.add(name.python_string, initial_value, is_const=False)
        return objects.null

    def _add_const(self, name, initial_value):
        assert isinstance(name, objects.String)
        self.namespace.add(name.python_string, initial_value, is_const=True)
        return objects.null

    def _set_var(self, name, value):
        assert isinstance(name, objects.String)
        self.namespace.set(name.python_string, value)
        return objects.null

    def _get_var(self, name):
        assert isinstance(name, objects.String)
        return self.namespace.get(name.python_string)
        return objects.null

    # handy-dandy helper method to avoid having to import this file to
    # objects.py (lol)
    def create_subcontext(self):
        return Context(self)


class Interpreter:

    def __init__(self):
        self.builtin_context = Context(None)
        objects.add_real_builtins(self.builtin_context.namespace)
        self._add_fake_builtins()
        self.global_context = Context(self.builtin_context)

    def _add_fake_builtins(self):
        # TODO: handle this path better
        path = os.path.join(
            os.path.dirname(os.path.abspath(__file__)), '..',
            'lib', 'fake_builtins.simple')
        with open(path, 'r', encoding='ascii') as file:
            content = file.read()
        tokens = tokenizer.tokenize(content)
        for ast_statement in ast_tree.parse(tokens):
            self.execute(ast_statement, self.builtin_context)

    def evaluate(self, ast_expression, context):
        if isinstance(ast_expression, ast_tree.String):
            return objects.String(ast_expression.python_string)

        if isinstance(ast_expression, ast_tree.GetVar):
            return context.namespace.get(ast_expression.varname)

        if isinstance(ast_expression, ast_tree.GetAttr):
            obj = self.evaluate(ast_expression.object, context)
            return obj.attributes.get(ast_expression.attribute)

        if isinstance(ast_expression, ast_tree.Call):
            func = self.evaluate(ast_expression.func, context)
            args = [self.evaluate(arg, context) for arg in ast_expression.args]
            return func.call(args)

        if isinstance(ast_expression, ast_tree.Code):
            return objects.Code(self, context, ast_expression.statements)

        raise RuntimeError(        # pragma: no cover
            "don't know how to evaluate " + repr(ast_expression))

    def execute(self, ast_statement, context):
        if isinstance(ast_statement, ast_tree.Call):
            self.evaluate(ast_statement, context)

        elif isinstance(ast_statement, ast_tree.CreateVar):
            initial_value = self.evaluate(ast_statement.value, context)
            context.namespace.add(ast_statement.varname, initial_value,
                                  ast_statement.is_const)

        elif isinstance(ast_statement, ast_tree.CreateAttr):
            obj = self.evaluate(ast_statement.object, context)
            initial_value = self.evaluate(ast_statement.value, context)
            obj.attributes.add(ast_statement.attribute, initial_value,
                               ast_statement.is_const)

        elif isinstance(ast_statement, ast_tree.SetAttr):
            obj = self.evaluate(ast_statement.object, context)
            value = self.evaluate(ast_statement.value, context)
            obj.attributes.set(ast_statement.attribute, value)

        elif isinstance(ast_statement, ast_tree.SetVar):
            value = self.evaluate(ast_statement.value, context)
            context.namespace.set(ast_statement.varname, value)

        elif isinstance(ast_statement, ast_tree.Return):
            value = self.evaluate(ast_statement.value, context)
            raise objects.ReturnAValue(value)

        else:   # pragma: no cover
            raise RuntimeError(
                "don't know how to execute " + repr(ast_statement))
