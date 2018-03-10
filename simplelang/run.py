import collections
import functools

from simplelang import ast_tree, objects


# usually Context(None) means that the builtin context should be
# used, but it can't be used yet if it hasn't been created
builtin_context = None


class Context:

    def __init__(self, parent_context):
        if parent_context is None:
            parent_context = builtin_context
        self.parent_context = parent_context

        if parent_context is None:
            self.namespace = objects.Namespace('variable')
        else:
            self.namespace = objects.Namespace(
                'variable', parent_context.namespace)


builtin_context = Context(None)
objects.add_builtins(builtin_context.namespace)


class Interpreter:

    def __init__(self):
        self.global_context = Context(builtin_context)

    def evaluate(self, ast_expression, context) -> objects.Object:
        if isinstance(ast_expression, ast_tree.String):
            return objects.String(ast_expression.python_string)
        if isinstance(ast_expression, ast_tree.GetVar):
            return context.namespace.get(ast_expression.varname)
        if isinstance(ast_expression, ast_tree.GetAttr):
            obj = self.evaluate(ast_expression.object, context)
            return obj.attributes.get(ast_expression.attribute)

    def execute(self, ast_statements, context):
        for statement in ast_statements:
            if isinstance(statement, ast_tree.Call):
                func = self.evaluate(statement.func, context)
                args = [self.evaluate(arg, context) for arg in statement.args]
                func.call(args)
            elif isinstance(statement, ast_tree.CreateVar):
                initial_value = self.evaluate(statement.value, context)
                context.namespace.add(statement.varname, initial_value,
                                      statement.is_const)
            elif isinstance(statement, ast_tree.CreateAttr):
                obj = self.evaluate(statement.object, context)
                initial_value = self.evaluate(statement.value, context)
                obj.attributes.add(statement.attribute, initial_value,
                                   statement.is_const)
            elif isinstance(statement, ast_tree.SetAttr):
                obj = self.evaluate(statement.object, context)
                value = self.evaluate(statement.value, context)
                obj.attributes.set(statement.attribute, value)
            elif isinstance(statement, ast_tree.SetVar):
                value = self.evaluate(statement.value, context)
                context.namespace.set(statement.varname, value)
            else:   # pragma: no cover
                raise RuntimeError("unknown statement: " + repr(statement))
