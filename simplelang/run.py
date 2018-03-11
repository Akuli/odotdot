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

    # handy-dandy helper method to avoid having to import this file to
    # objects.py (lol)
    def create_subcontext(self):
        return Context(self)


builtin_context = Context(None)
objects.add_builtins(builtin_context.namespace)


class Interpreter:

    def __init__(self):
        self.global_context = Context(builtin_context)

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
            func = self.evaluate(ast_statement.func, context)
            args = [self.evaluate(arg, context) for arg in ast_statement.args]
            func.call(args)
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
