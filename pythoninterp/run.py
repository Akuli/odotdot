import collections
import functools
import os

from . import tokenizer, ast_tree, objects


def _create_scope_info():
    def setup(this, parent_scope):
        this.attributes['parent_scope'] = parent_scope
        this.attributes['local_vars'] = objects.Öbject(objects.mapping_info)
        this.attributes['local_vars'].call_method(
            'setup', objects.new_array([]))

    def set_var_where_defined(this, name, value):
        if name in this.attributes['local_vars'].python_dict:
            this.attributes['local_vars'].python_dict[name] = value
        elif this.attributes['parent_scope'] is not objects.null:
            this.attributes['parent_scope'].call_method(
                'set_var_where_defined', name, value)
        else:
            raise ValueError("no variable named '%s'" % name.python_string)

    def get_var(this, name):
        try:
            return this.attributes['local_vars'].python_dict[name]
        except KeyError:
            if this.attributes['parent_scope'] is objects.null:
                raise ValueError("no variable named '%s'" % name.python_string)
            return this.attributes['parent_scope'].call_method(
                'get_var', name)

    # TODO: get rid of this???
    def create_subscope(this):
        scope = objects.Öbject(scope_info)
        scope.call_method('setup', this)
        return scope

    return objects.ClassInfo(objects.object_info, {
        'setup': setup,
        'set_var_where_defined': set_var_where_defined,
        'get_var': get_var,
        'create_subscope': create_subscope,
    })


scope_info = _create_scope_info()


class Interpreter:

    def __init__(self):
        self._stack = []

        self.builtin_scope = objects.Öbject(scope_info)
        self.builtin_scope.call_method('setup', objects.null)

        objects.add_real_builtins(self.builtin_scope)
        self._add_fake_builtins()
        self.global_scope = self.builtin_scope.call_method(
            'create_subscope')

    def _add_fake_builtins(self):
        # TODO: handle this path better
        path = os.path.abspath(os.path.join(
            os.path.dirname(os.path.abspath(__file__)),
            '..', 'stdlib', 'fake_builtins.ö'))
        with open(path, 'r', encoding='ascii') as file:
            content = file.read()

        tokens = tokenizer.tokenize(content, path)
        for ast_statement in ast_tree.parse(tokens):
            self.execute(ast_statement, self.builtin_scope)

    def evaluate(self, ast_expression, scope):
        if isinstance(ast_expression, ast_tree.String):
            return objects.new_string(ast_expression.python_string)

        if isinstance(ast_expression, ast_tree.Integer):
            return objects.new_integer(ast_expression.python_int)

        if isinstance(ast_expression, ast_tree.Array):
            elements = [self.evaluate(element, scope)
                        for element in ast_expression.elements]
            return objects.new_array(elements)

        if isinstance(ast_expression, ast_tree.GetVar):
            varname = objects.new_string(ast_expression.varname)
            return scope.call_method('get_var', varname)

        if isinstance(ast_expression, ast_tree.GetAttr):
            obj = self.evaluate(ast_expression.object, scope)
            return obj.attributes[ast_expression.attributename]

        if isinstance(ast_expression, ast_tree.GetMethod):
            obj = self.evaluate(ast_expression.object, scope)
            return obj.methods[ast_expression.methodname]

        if isinstance(ast_expression, ast_tree.Call):
            func = self.evaluate(ast_expression.func, scope)
            args = [self.evaluate(arg, scope) for arg in ast_expression.args]
            return func.call(scope, *args)

        if isinstance(ast_expression, ast_tree.Block):
            return objects.new_block(self, scope, ast_expression.statements)

        raise RuntimeError(        # pragma: no cover
            "don't know how to evaluate " + repr(ast_expression))

    def execute(self, ast_statement, scope):
        self._stack.append(ast_statement)

        try:
            if isinstance(ast_statement, ast_tree.Call):
                self.evaluate(ast_statement, scope)

            elif isinstance(ast_statement, ast_tree.CreateVar):
                varname = objects.new_string(ast_statement.varname)
                value = self.evaluate(ast_statement.value, scope)
                scope.attributes['local_vars'].python_dict[varname] = value

            elif isinstance(ast_statement, ast_tree.SetVar):
                value = self.evaluate(ast_statement.value, scope)
                scope.call_method(
                    'set_var_where_defined',
                    objects.new_string(ast_statement.varname), value)

            elif isinstance(ast_statement, ast_tree.SetAttr):
                obj = self.evaluate(ast_statement.object, scope)
                value = self.evaluate(ast_statement.value, scope)
                obj.attributes[ast_statement.attribute] = value

            else:   # pragma: no cover
                raise RuntimeError(
                    "don't know how to execute " + repr(ast_statement))

        except objects.Errör as e:
            if e.stack is None:
                e.stack = self._stack.copy()
            raise e

        finally:
            popped = self._stack.pop()
            assert popped is ast_statement
