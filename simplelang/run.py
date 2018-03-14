import collections
import functools
import os

from simplelang import tokenizer, ast_tree, objects


class Context(objects.Object):

    def __init__(self, parent_context):
        if parent_context is None:
            parent_context = objects.null
        super().__init__([parent_context])

    def _setup(self, parent_context):
        self.attributes['parent_context'] = parent_context
        self.local_vars = {}

    def _set_var_locally(self, name, value):
        self.local_vars[name.python_string] = value

    def _set_var_where_defined(self, name, value):
        if name.python_string in self.local_vars:
            self.local_vars[name.python_string] = value
        elif self.attributes['parent_context'] is not objects.null:
            self.attributes['parent_context'].call_method(
                'set_var_where_defined', name, value)
        else:
            raise ValueError("no variable named '%s'" % name.python_string)

    def _get_var(self, name):
        try:
            return self.local_vars[name.python_string]
        except KeyError:
            if self.attributes['parent_context'] is objects.null:
                raise ValueError("no variable named '%s'" % name.python_string)
            return self.attributes['parent_context'].call_method(
                'get_var', name)

    def _delete_local_var(self, name):
        try:
            del self.local_vars[name]
        except KeyError:
            raise ValueError(
                "no local variable named '%s'" % name.python_string)

    def _create_subcontext(self):
        return Context(self)

    class_info = objects.ClassInfo(objects.Object.class_info, {
        'setup': objects.Function(_setup),
        'set_var_locally': objects.Function(_set_var_locally),
        'set_var_where_defined': objects.Function(_set_var_where_defined),
        'get_var': objects.Function(_get_var),
        'delete_local_var': objects.Function(_delete_local_var),
        'create_subcontext': objects.Function(_create_subcontext),
    })


class Interpreter:

    def __init__(self):
        self.builtin_context = Context(None)
        objects.add_real_builtins(self.builtin_context)
        self._add_fake_builtins()
        self.global_context = Context(self.builtin_context)

    def _add_fake_builtins(self):
        # TODO: handle this path better
        path = os.path.join(
            os.path.dirname(os.path.abspath(__file__)), '..',
            'stdlib', 'fake_builtins.simple')
        with open(path, 'r', encoding='ascii') as file:
            content = file.read()
        tokens = tokenizer.tokenize(content)
        for ast_statement in ast_tree.parse(tokens):
            self.execute(ast_statement, self.builtin_context)

    def evaluate(self, ast_expression, context):
        if isinstance(ast_expression, ast_tree.String):
            return objects.String(ast_expression.python_string)

        if isinstance(ast_expression, ast_tree.Integer):
            return objects.Integer(ast_expression.python_int)

        if isinstance(ast_expression, ast_tree.Array):
            elements = [self.evaluate(element, context)
                        for element in ast_expression.elements]
            return objects.Array(elements)

        if isinstance(ast_expression, ast_tree.GetVar):
            varname = objects.String(ast_expression.varname)
            return context.call_method('get_var', varname)

        if isinstance(ast_expression, ast_tree.GetAttr):
            obj = self.evaluate(ast_expression.object, context)
            return obj.attributes[ast_expression.attribute]

        if isinstance(ast_expression, ast_tree.Call):
            func = self.evaluate(ast_expression.func, context)
            args = [self.evaluate(arg, context) for arg in ast_expression.args]
            return func.call(args)

        if isinstance(ast_expression, ast_tree.Block):
            return objects.Block(self, context, ast_expression.statements)

        raise RuntimeError(        # pragma: no cover
            "don't know how to evaluate " + repr(ast_expression))

    def execute(self, ast_statement, context):
        if isinstance(ast_statement, ast_tree.Call):
            self.evaluate(ast_statement, context)

        elif isinstance(ast_statement, ast_tree.CreateVar):
            initial_value = self.evaluate(ast_statement.value, context)
            context.local_vars[ast_statement.varname] = initial_value

        elif isinstance(ast_statement, ast_tree.SetVar):
            value = self.evaluate(ast_statement.value, context)
            context.call_method('set_var_where_defined',
                                objects.String(ast_statement.varname), value)

        elif isinstance(ast_statement, ast_tree.SetAttr):
            obj = self.evaluate(ast_statement.object, context)
            value = self.evaluate(ast_statement.value, context)
            obj.attributes[ast_statement.attribute] = value

        else:   # pragma: no cover
            raise RuntimeError(
                "don't know how to execute " + repr(ast_statement))
