import argparse
import traceback

from ö import tokenizer, ast_tree, run, objects, stack_trace


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('file', type=argparse.FileType('r'),
                        nargs=argparse.OPTIONAL)
    args = parser.parse_args()

    interpreter = run.Interpreter()

    if args.file is None:
        # REPL
        while True:
            code = input('>> ')
            tokens = tokenizer.tokenize(code, '<stdin>')
            ast_statements = ast_tree.parse(tokens)

            try:
                for statement in ast_statements:
                    interpreter.execute(statement, interpreter.global_context)
            except objects.Errör as e:
                stack_trace.print_stack_trace(e)
            except Exception:   # TODO: get rid of this and let things crash
                traceback.print_exc()

    else:
        with args.file:
            content = args.file.read()

        tokens = tokenizer.tokenize(content, args.file.name)
        ast_statements = ast_tree.parse(tokens)

        try:
            for statement in ast_statements:
                interpreter.execute(statement, interpreter.global_context)
        except objects.Errör as e:
            stack_trace.print_stack_trace(e)
        except objects.ReturnAValue:
            raise ValueError("unexpected return")


if __name__ == '__main__':     # pragma: no cover
    main()
